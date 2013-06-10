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

#ifndef PEAK_PACKET_H
#define PEAK_PACKET_H

#ifndef IPPROTO_IPV4
#define IPPROTO_IPV4	IPPROTO_IPIP
#endif /* !IPPROTO_IPV4 */

#ifndef IPPROTO_OSPFIGP
#define IPPROTO_OSPFIGP	89
#endif /* !IPPROTO_OSPFIGP */

#ifndef IPPROTO_IPEIP
#define IPPROTO_IPEIP	94
#endif /* !IPPROTO_IPEIP */

#define SRC(x, y)	((x) + !!(y))
#define DST(x, y)	((x) + !(y))
#define LOWER		0
#define UPPER		1

struct peak_packet {
	union {
		struct ether_header *eth;
		unsigned char *raw;
	} mac;
	union {
		unsigned char *raw;
		struct ip *iph;
	} net;
	union {
		unsigned char *raw;
		struct tcphdr *th;
		struct udphdr *uh;
	} flow;
	union {
		unsigned char *raw;
	} app;
	uint32_t mac_len;
	uint16_t mac_type;
	uint16_t net_len;
	uint8_t net_hlen;
	uint8_t net_type;
	uint16_t flow_hlen;
	uint16_t flow_len;
	uint16_t app_len;
	uint16_t flow_sport;
	uint16_t flow_dport;
	uint16_t reserved[2];
};

#endif /* !PEAK_PACKET_H */
