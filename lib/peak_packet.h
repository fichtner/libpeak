/*
 * Copyright (c) 2012-2014 Franco Fichtner <franco@packetwerk.com>
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

#ifndef IFNAMSIZ
#define IFNAMSIZ	16
#endif /* !IFNAMSIZ */

#ifndef IPPROTO_OSPFIGP
#define IPPROTO_OSPFIGP	89
#endif /* !IPPROTO_OSPFIGP */

#ifndef IPPROTO_IPEIP
#define IPPROTO_IPEIP	94
#endif /* !IPPROTO_IPEIP */

#ifndef IPPROTO_L2TP
#define IPPROTO_L2TP	115
#endif /* !IPPROTO_L2TP */

#ifndef linux
#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP	132
#endif /* IPPROTO_SCTP */
#endif /* !linux */

#ifndef IPPROTO_HIP
#define IPPROTO_HIP	139
#endif /* IPPROTO_HIP */

#ifndef IPPROTO_SHIM6
#define IPPROTO_SHIM6	140
#endif /* IPPROTO_SHIM6 */

#ifndef LINKTYPE_NULL
#define LINKTYPE_NULL		0
#endif	/* !LINKTYPE_NULL */

#ifndef LINKTYPE_ETHERNET
#define LINKTYPE_ETHERNET	1
#endif /* !LINKTYPE_ETHERNET */

#ifndef LINKTYPE_LINUX_SLL
#define LINKTYPE_LINUX_SLL	113
#endif /* !LINKTYPE_LINUX_SLL */

struct sll_header {
	uint16_t sll_pkttype;	/* packet type */
	uint16_t sll_hatype;	/* link-layer address type */
	uint16_t ssl_halen;	/* link-layer address length */
	uint8_t ssl_addr[8];	/* link-layer address */
	uint16_t sll_protocol;	/* protocol */
} __packed;

struct vlan_header {
	uint8_t ether_dhost[6];	/* destination mac address */
	uint8_t ether_shost[6];	/* source mac address */
	uint16_t vlan_tpid;	/* vlan tag protocol identifier */
	uint16_t vlan_tci;	/* vlan tag control information */
	uint16_t ether_type;	/* protocol */
} __packed;

#define LOWER		0
#define UPPER		1

struct peak_packet {
	union {
		struct ether_header *eth;
		struct vlan_header *vlan;
		struct sll_header *sll;
		unsigned char *raw;
	} mac;
	union {
		struct ip6_hdr *ip6h;
		unsigned char *raw;
		struct ip *iph;
	} net;
	union {
		unsigned char *raw;
#if defined(__FreeBSD__) || defined(__linux__)
		struct icmphdr *ih;
#else /* !__FreeBSD__ && !__linux__ */
		struct icmp *ih;
#endif /* __FreeBSD__ || __linux__ */
		struct tcphdr *th;
		struct udphdr *uh;
	} flow;
	union {
		unsigned char *raw;
	} app;
	uint32_t mac_len;
	uint16_t mac_type;
	uint16_t mac_vlan;
	struct netaddr net_saddr;
	struct netaddr net_daddr;
	uint16_t net_family;
	uint16_t net_len;
	uint8_t net_hlen;
	uint8_t net_type;
	uint16_t flow_hlen;
	uint16_t flow_len;
	uint16_t app_len;
	uint16_t flow_sport;
	uint16_t flow_dport;
	uint32_t link_type;
};

unsigned int	 peak_packet_parse(struct peak_packet *, void *,
		     unsigned int, unsigned int);
const char	*peak_packet_mac(const struct peak_packet *);
const char	*peak_packet_net(const struct peak_packet *);

#endif /* !PEAK_PACKET_H */
