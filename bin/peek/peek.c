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

output_init();

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
peek_packet(struct peak_tracks *peek, const timeslice_t *timer,
    void *buf, unsigned int len, unsigned int type)
{
	struct peak_packet packet;
	struct peak_track *flow;
	struct peak_track _flow;
	struct netmap src, dst;
	char tsbuf[40];

	PACKET_LINK(&packet, buf, len, type);

	switch (packet.mac_type) {
	case ETHERTYPE_IP:
		netmap4(&src, packet.net.iph->ip_src.s_addr);
		netmap4(&dst, packet.net.iph->ip_dst.s_addr);
		packet.net_len = be16dec(&packet.net.iph->ip_len);
		packet.net_hlen = packet.net.iph->ip_hl << 2;
		packet.net_type = packet.net.iph->ip_p;
		break;
	case ETHERTYPE_IPV6:
		netmap6(&src, &packet.net.ip6h->ip6_src);
		netmap6(&dst, &packet.net.ip6h->ip6_dst);
		peek_packet_ipv6(&packet);
		break;
	default:
		/* here be dragons */
		return;
	}

	packet.flow.raw = packet.net.raw + packet.net_hlen;
	packet.flow_len = packet.net_len - packet.net_hlen;

	switch (packet.net_type) {
	case IPPROTO_TCP:
		if (peak_tcp_prepare(&packet)) {
			return;
		}
		break;
	case IPPROTO_UDP:
		if (peak_udp_prepare(&packet)) {
			return;
		}
		break;
	default:
		break;
	}

	TRACK_KEY(&_flow, src, dst, packet.flow_sport, packet.flow_dport,
	   packet.net_type);

	flow = peak_track_acquire(peek, &_flow);
	if (!flow) {
		panic("tracker should never be empty\n");
	}

	{
		const unsigned int dir = !!netcmp(&src, &flow->user[LOWER]);

		if (!flow->li[dir]) {
			flow->li[dir] = peak_li_get(&packet);
		}
	}

	pout("flow: %zu, ip_type: %hhu, ip_len: %u, app: %s"", time: %s\n",
	    flow->id, packet.net_type, packet.net_len,
	    peak_li_name(LI_MERGE(flow->li)),
	    strftime(tsbuf, sizeof(tsbuf),
	    "%a %F %T", &timer->gmt) ? tsbuf : "???");
}

static void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s file\n", __progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	struct peak_tracks *peek;
	struct peak_load *trace;
	timeslice_t timer;

	if (argc < 2) {
		usage();
	}

	trace = peak_load_init(argv[1]);
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
