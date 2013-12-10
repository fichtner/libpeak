/*
 * Copyright (c) 2013 Franco Fichtner <franco@packetwerk.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <peak.h>
#if defined(__OpenBSD__) || defined(__FreeBSD__)
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#else /* !__OpenBSD__ && !__FreeBSD__ */
#include <net/ethernet.h>
#endif /* __OpenBSD__ || __FreeBSD__ */
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <unistd.h>

output_init();

enum {
	USE_APP,
	USE_APP_LEN,
	USE_FLOW,
	USE_IP_LEN,
	USE_IP_TYPE,
	USE_TIME,
	USE_MAX		/* last element */
};

static unsigned int use_print[USE_MAX];
static unsigned int use_count = 0;

static void
peek_packet_ipv6(struct peak_packet *packet)
{
	struct ip6_hdr *i6h = packet->net.ip6h;
	struct ip6_ext *next_hdr = (struct ip6_ext *)(i6h + 1);
	uint8_t next_type = i6h->ip6_nxt;

	packet->net_len = sizeof(*i6h) + be16dec(&i6h->ip6_plen);
	packet->net_hlen = sizeof(*i6h);

	for (;;) {
		switch (next_type) {
		case IPPROTO_HOPOPTS:
			/* FALLTHROUGH */
		case IPPROTO_ROUTING:
			/* FALLTHROUGH */
		case IPPROTO_DSTOPTS:
			/* FALLTHROUGH */
		case IPPROTO_ESP:
			/* FALLTHROUGH */
		case IPPROTO_AH:
			/* variable extension header */
			if (next_hdr->ip6e_len % 8) {
				panic("invalid ipv6 extension header"
				    "size %hhu\n", next_hdr->ip6e_len);
			}
			packet->net_hlen += next_hdr->ip6e_len;
			next_type = next_hdr->ip6e_nxt;
			next_hdr += next_hdr->ip6e_len >> 1;
			break;
		case IPPROTO_FRAGMENT:
			/* fixed extension header */
			packet->net_hlen += sizeof(struct ip6_frag);
			next_type = next_hdr->ip6e_nxt;
			next_hdr += sizeof(struct ip6_frag) >> 1;
			break;
		case IPPROTO_NONE:
			/* FALLTHROUGH */
		default:
			/* no more extension headers  */
			packet->net_type = next_type;
			return;
		}
	}

	/* NOTREACHED */
}

static void
peek_report(const struct peak_packet *packet, const struct peak_track *flow,
    const timeslice_t *timer)
{
	unsigned int i;

	for (i = 0; i < use_count; ++i) {
		if (i) {
			pout(", ");
		}

		switch (use_print[i]) {
		case USE_APP:
			pout("app: %s", peak_li_name(LI_MERGE(flow->li)));
			break;
		case USE_APP_LEN:
			pout("app_len: %hu", packet->app_len);
			break;
		case USE_FLOW:
			pout("flow: %zu", flow->id);
			break;
		case USE_IP_LEN:
			pout("ip_len: %hu", packet->net_len);
			break;
		case USE_IP_TYPE:
			pout("ip_type: %hhu", packet->net_type);
			break;
		case USE_TIME: {
			char tsbuf[40];
			pout("time: %s", strftime(tsbuf, sizeof(tsbuf),
			    "%a %F %T", &timer->gmt) ? tsbuf : "???");
			break;
		}
		default:
			break;
		}
	}

	pout("\n");
}

static void
peek_packet(struct peak_tracks *peek, const timeslice_t *timer,
    void *buf, unsigned int len, unsigned int type)
{
	struct peak_packet stackptr(packet);
	struct peak_track *flow;
	struct peak_track _flow;

	PACKET_LINK(packet, buf, len, type);

	switch (packet->mac_type) {
	case ETHERTYPE_IP:
		netaddr4(&packet->net_saddr, packet->net.iph->ip_src.s_addr);
		netaddr4(&packet->net_daddr, packet->net.iph->ip_dst.s_addr);
		packet->net_len = be16dec(&packet->net.iph->ip_len);
		packet->net_hlen = packet->net.iph->ip_hl << 2;
		packet->net_type = packet->net.iph->ip_p;
		break;
	case ETHERTYPE_IPV6:
		netaddr6(&packet->net_saddr, &packet->net.ip6h->ip6_src);
		netaddr6(&packet->net_daddr, &packet->net.ip6h->ip6_dst);
		peek_packet_ipv6(packet);
		break;
	default:
		/* here be dragons */
		return;
	}

	packet->flow.raw = packet->net.raw + packet->net_hlen;
	packet->flow_len = packet->net_len - packet->net_hlen;

	switch (packet->net_type) {
	case IPPROTO_TCP:
		if (peak_tcp_prepare(packet)) {
			return;
		}
		break;
	case IPPROTO_UDP:
		if (peak_udp_prepare(packet)) {
			return;
		}
		break;
	default:
		break;
	}

	TRACK_KEY(&_flow, packet);

	flow = peak_track_acquire(peek, &_flow);
	if (!flow) {
		panic("tracker should never be empty\n");
	}

	{
		const unsigned int dir =
		    !!netcmp(&packet->net_saddr, &flow->addr[LOWER]);

		if (!flow->li[dir]) {
			flow->li[dir] = peak_li_get(packet);
		}
	}

	peek_report(packet, flow, timer);
}

static void
usage(void)
{
	extern char *__progname;

	perr("usage: %s [-AafNnt] file\n", __progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	struct peak_tracks *peek;
	struct peak_load *trace;
	timeslice_t timer;
	int c;

	while ((c = getopt(argc, argv, "AafNnt")) != -1) {
		switch (c) {
		case 'A':
			use_print[use_count++] = USE_APP_LEN;
			break;
		case 'a':
			use_print[use_count++] = USE_APP;
			break;
		case 'f':
			use_print[use_count++] = USE_FLOW;
			break;
		case 'N':
			use_print[use_count++] = USE_IP_LEN;
			break;
		case 'n':
			use_print[use_count++] = USE_IP_TYPE;
			break;
		case 't':
			use_print[use_count++] = USE_TIME;
			break;
		default:
			usage();
			/* NOTREACHED */
		}

		if (use_count > USE_MAX) {
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage();
		/* NOTREACHED */
	}

	if (!use_count) {
		/* set the default output (as used by tests) */
		use_print[use_count++] = USE_FLOW;
		use_print[use_count++] = USE_IP_TYPE;
		use_print[use_count++] = USE_IP_LEN;
		use_print[use_count++] = USE_APP;
		use_print[use_count++] = USE_TIME;
	}

	trace = peak_load_init(argv[0]);
	if (!trace) {
		panic("cannot init file loader\n");
	}

	peek = peak_track_init(1000);
	if (!peek) {
		panic("cannot init flow tracker\n");
	}

	TIMESLICE_INIT(&timer);

	if (peak_load_next(trace)) {
		TIMESLICE_NORMALISE(&timer, trace->ts_ms);

		do {
			TIMESLICE_ADVANCE(&timer, trace->ts_unix,
			    trace->ts_ms);
			peek_packet(peek, &timer, trace->buf, trace->len,
			    trace->ll);
		} while (peak_load_next(trace));
	}

	peak_track_exit(peek);
	peak_load_exit(trace);

	return (0);
}
