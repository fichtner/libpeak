/*
 * Copyright (c) 2012-2014 Franco Fichtner <franco@packetwerk.com>
 * Copyright (c) 2014 Ulrich Klein <ulrich@packetwerk.com>
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
#if defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#else /* !__OpenBSD__ && !__NetBSD__ */
#include <net/ethernet.h>
#endif /* __OpenBSD__ || __NetBSD__ */
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#ifndef TCP_MAXHLEN
#define TCP_MAXHLEN	(0xf<<2)	/* max length of header in bytes */
#endif /* !TCP_MAXHLEN */

const char *
peak_packet_mac(const struct peak_packet *self)
{
	const char *ret;

	switch (self->mac_type) {
	case ETHERTYPE_IP:
		ret = "ip";
		break;
	case ETHERTYPE_IPV6:
		ret = "ipv6";
		break;
	default:
		ret = "xxx";
		break;
	}

	return (ret);
}
const char *
peak_packet_net(const struct peak_packet *self)
{
	const char *ret;

	switch (self->net_type) {
	case IPPROTO_ICMP:
		ret = "icmp";
		break;
	case IPPROTO_IGMP:
		ret = "igmp";
		break;
	case IPPROTO_IPV4:
		ret = "ipv4";
		break;
	case IPPROTO_TCP:
		ret = "tcp";
		break;
	case IPPROTO_UDP:
		ret = "udp";
		break;
	case IPPROTO_IPV6:
		ret = "ipv6";
		break;
	case IPPROTO_GRE:
		ret = "gre";
		break;
	case IPPROTO_NONE:
		ret = "none";
		break;
	case IPPROTO_ICMPV6:
		ret = "icmpv6";
		break;
	case IPPROTO_IPEIP:
		ret = "ipeip";
		break;
	case IPPROTO_OSPFIGP:
		ret = "ospf";
		break;
	case IPPROTO_L2TP:
		ret = "l2tp";
		break;
	case IPPROTO_SCTP:
		ret = "sctp";
		break;
	case IPPROTO_PIM:
		ret = "pim";
		break;
	default:
		ret = "xxx";
		break;
	}

	return (ret);
}

static unsigned int
peak_packet_ipv6(struct peak_packet *self)
{
	struct ip6_hdr *i6h = self->net.ip6h;
	struct ip6_ext *next_hdr = (struct ip6_ext *)(i6h + 1);
	uint8_t next_type = i6h->ip6_nxt;

	self->net_len = sizeof(*i6h) + be16dec(&i6h->ip6_plen);
	self->net_hlen = sizeof(*i6h);

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
				alert("invalid ipv6 extension header"
				    "size %hhu\n", next_hdr->ip6e_len);
				return (1);
			}
			self->net_hlen += next_hdr->ip6e_len;
			next_type = next_hdr->ip6e_nxt;
			next_hdr += next_hdr->ip6e_len >> 1;
			break;
		case IPPROTO_FRAGMENT:
			/* fixed extension header */
			self->net_hlen += sizeof(struct ip6_frag);
			next_type = next_hdr->ip6e_nxt;
			next_hdr += sizeof(struct ip6_frag) >> 1;
			break;
		case IPPROTO_NONE:
			/* FALLTHROUGH */
		default:
			/* no more extension headers  */
			self->net_type = next_type;
			return (0);
		}
	}

	/* NOTREACHED */
}

static unsigned int
peak_packet_udp(struct peak_packet *self)
{
	struct udphdr *uh = self->flow.uh;
	unsigned int flow_hlen = sizeof(*uh);

	if (unlikely(flow_hlen > self->flow_len)) {
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

	self->app_len = self->flow_len - flow_hlen;
	self->app.raw = self->flow.raw + flow_hlen;
	self->flow_sport = be16dec(&uh->uh_sport);
	self->flow_dport = be16dec(&uh->uh_dport);
	self->flow_hlen = flow_hlen;

	return (0);
}

static unsigned int
peak_packet_tcp(struct peak_packet *self)
{
	struct tcphdr *th = self->flow.th;
	unsigned int flow_hlen, flags;

	flow_hlen = th->th_off << 2;
	if (unlikely(flow_hlen < sizeof(*th) ||
	    flow_hlen > TCP_MAXHLEN)) {
		return (1);
	}

	if (unlikely(flow_hlen > self->flow_len)) {
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

	self->app.raw = self->flow.raw + flow_hlen;
	self->app_len = self->flow_len - flow_hlen;
	self->flow_sport = be16dec(&th->th_sport);
	self->flow_dport = be16dec(&th->th_dport);
	self->flow_hlen = flow_hlen;

	return (0);
}
unsigned int
peak_packet_parse(struct peak_packet *self, void *buf, unsigned int len,
    unsigned int type)
{
	memset(self, 0, sizeof(*self));

	self->link_type = type;
	self->mac.raw = buf;
	self->mac_len = len;

	switch (type) {
	case LINKTYPE_ETHERNET: {
		uint16_t m_type = be16dec(&self->mac.eth->ether_type);
		if (m_type == ETHERTYPE_VLAN) {
			self->mac_type =
			    be16dec(&self->mac.vlan->ether_type);
			self->mac_vlan = 0x0FFF &
			    be16dec(&self->mac.vlan->vlan_tci);
			self->net.raw = self->mac.raw +
			    sizeof(*self->mac.vlan);
		}  else {
			self->mac_type = m_type;
			self->net.raw = self->mac.raw +
			    sizeof(*self->mac.eth);
		}
		break;
	}
	case LINKTYPE_LINUX_SLL:
		self->mac_type = be16dec(&self->mac.sll->sll_protocol);
		self->net.raw = self->mac.raw + sizeof(*self->mac.sll);
		break;
	default:
		alert("unknown link type %u\n", type);
		return (1);
	}

	switch (self->mac_type) {
	case ETHERTYPE_IP: {
		struct ip *iph = self->net.iph;
		const unsigned int ip_hlen = iph->ip_hl << 2;
		const unsigned int ip_len = be16dec(&iph->ip_len);

		if (unlikely(iph->ip_v != IPVERSION)) {
			return (1);
		}

		if (unlikely(ip_hlen < sizeof(*iph))) {
			return (1);
		}

		/* XXX check for fragmentation */

		if (unlikely(ip_len < ip_hlen)) {
			return (1);
		}

		/* XXX check for total length */

		netaddr4(&self->net_saddr, iph->ip_src.s_addr);
		netaddr4(&self->net_daddr, iph->ip_dst.s_addr);
		self->net_family = AF_INET;

		self->net_type = iph->ip_p;
		self->net_hlen = ip_hlen;
		self->net_len = ip_len;

		break;
	}
	case ETHERTYPE_IPV6: {
		struct ip6_hdr *ip6h = self->net.ip6h;

		netaddr6(&self->net_saddr, &ip6h->ip6_src);
		netaddr6(&self->net_daddr, &ip6h->ip6_dst);
		self->net_family = AF_INET6;

		if (unlikely(peak_packet_ipv6(self))) {
			return (1);
		}

		break;
	}
	default:
		return (0);
	}

	/* calculate next header start and length */
	self->flow.raw = self->net.raw + self->net_hlen;
	self->flow_len = self->net_len - self->net_hlen;

	switch (self->net_type) {
	case IPPROTO_TCP:
		if (peak_packet_tcp(self)) {
			return (1);
		}
		break;
	case IPPROTO_UDP:
		if (peak_packet_udp(self)) {
			return (1);
		}
		break;
	case IPPROTO_ICMP:
		if (unlikely(self->flow_len < ICMP_MINLEN)) {
			return (1);
		}
		/*
		 * XXX ICMP handling is naive.  Pimp features.
		 * Also need to add ICMPv6.
		 */
		self->app_len = self->flow_len - ICMP_MINLEN;
		self->app.raw = self->flow.raw + ICMP_MINLEN;
		self->flow_hlen = ICMP_MINLEN;
		break;
	default:
		break;
	}

	return (0);
}
