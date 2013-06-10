/*
 * Copyright (c) 2013 Franco Fichtner <franco@lastsummer.de>
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
#include <netinet/udp.h>
#include <netinet/tcp.h>

output_init();

static void
peek_packet(struct peak_tracks *peek, const timeslice_t *timer,
    void *buf, unsigned int len)
{
	struct peak_packet packet;
	struct peak_track *flow;
	struct peak_track _flow;
	struct netmap src, dst;
	char tsbuf[40];

	bzero(&packet, sizeof(packet));

	packet.mac.eth = buf;
	packet.mac_len = len;

	packet.net.raw = packet.mac.raw + sizeof(struct ether_header);
	packet.mac_type = be16dec(&packet.mac.eth->ether_type);

	if (packet.mac_type != ETHERTYPE_IP) {
		return;
	}

	packet.net_len = be16dec(&packet.net.iph->ip_len);
	packet.net_hlen = packet.net.iph->ip_hl << 2;
	packet.net_type = packet.net.iph->ip_p;

	netmap4(&src, packet.net.iph->ip_src.s_addr);
	netmap4(&dst, packet.net.iph->ip_dst.s_addr);

	packet.flow.raw = packet.net.raw + packet.net_hlen;
	packet.flow_len = packet.net_len - packet.net_hlen;

	switch (packet.net_type) {
	case IPPROTO_TCP:
		packet.flow_hlen = packet.flow.th->th_off << 2;
		packet.flow_sport = be16dec(&packet.flow.th->th_sport);
		packet.flow_dport = be16dec(&packet.flow.th->th_dport);
		break;
	case IPPROTO_UDP:
		packet.flow_hlen = sizeof(*packet.flow.uh);
		packet.flow_sport = be16dec(&packet.flow.uh->uh_sport);
		packet.flow_dport = be16dec(&packet.flow.uh->uh_dport);
		break;
	default:
		return;
	}

	packet.app.raw = packet.flow.raw + packet.flow_hlen;
	packet.app_len = packet.flow_len - packet.flow_hlen;

	TRACK_KEY(&_flow, src, dst, packet.flow_sport, packet.flow_dport,
	   packet.net_type);

	flow = peak_track_acquire(peek, &_flow);
	if (!flow) {
		panic("tracker should never be empty\n");
	}

	pout("flow: %zu, ip_type: %hhu, ip_len: %u, time: %s\n",
	    flow->id, packet.net_type, packet.net_len, strftime(tsbuf,
	    sizeof(tsbuf), "%a %F %T", &timer->gmt) ? tsbuf : "???");
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
			peek_packet(peek, &timer, trace->buf, trace->len);
		} while (peak_load_next(trace));
	}

	peak_track_exit(peek);
	peak_load_exit(trace);

	return (0);
}
