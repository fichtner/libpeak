/*
 * Copyright (c) 2012-2013 Franco Fichtner <franco@packetwerk.com>
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
#include <netinet/udp.h>

unsigned int
peak_udp_prepare(struct peak_packet *packet)
{
	struct udphdr *uh = packet->flow.uh;
	unsigned int flow_hlen = sizeof(*uh);

	if (unlikely(flow_hlen > packet->flow_len)) {
		return (1);
	}

	if (unlikely(!uh->uh_sport || !uh->uh_dport)) {
		return (1);
	}

	if (unlikely(!uh->uh_ulen || flow_hlen > be16dec(&uh->uh_ulen))) {
		return (1);
	}

	if (uh->uh_sum) {
		/* XXX verify checksum */
	}

	packet->app_len = packet->flow_len - flow_hlen;
	packet->app.raw = packet->flow.raw + flow_hlen;
	packet->flow_sport = be16dec(&uh->uh_sport);
	packet->flow_dport = be16dec(&uh->uh_dport);
	packet->flow_hlen = flow_hlen;

	return (0);
}
