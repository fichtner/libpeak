/*
 * Copyright (c) 2012 Franco Fichtner <franco@lastsummer.de>
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
#include <netinet/tcp.h>

#ifndef TCP_MAXHLEN
#define TCP_MAXHLEN	(0xf<<2)	/* max length of header in bytes */
#endif /* !TCP_MAXHLEN */

unsigned int
peak_tcp_prepare(struct peak_packet *packet)
{
	struct tcphdr *th = packet->flow.th;
	unsigned int flow_hlen, flags;

	flow_hlen = th->th_off << 2;
	if (unlikely(flow_hlen < sizeof(*th) ||
	    flow_hlen > TCP_MAXHLEN)) {
		return (1);
	}

	if (unlikely(flow_hlen > packet->flow_len)) {
		return (1);
	}

	if (unlikely(!th->th_sport || !th->th_dport)) {
		return (1);
	}

	/* XXX verify checksum */

	flags = th->th_flags;
	if (flags & TH_SYN) {
		if (flags & TH_RST) {
			return (1);
		}
	} else {
		if (!(flags & (TH_ACK|TH_RST))) {
			return (1);
		}
	}

	if (!(flags & TH_ACK)) {
		if (flags & (TH_FIN|TH_PUSH|TH_URG)) {
			return (1);
		}
	}

	packet->app.raw = packet->flow.raw + flow_hlen;
	packet->app_len = packet->flow_len - flow_hlen;
	packet->flow_sport = be16dec(&th->th_sport);
	packet->flow_dport = be16dec(&th->th_dport);
	packet->flow_hlen = flow_hlen;

	return (0);
}
