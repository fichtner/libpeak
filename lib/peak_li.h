/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
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

#ifndef PEAK_LI_H
#define PEAK_LI_H

enum {
	/*
	 * Unknown means there's not enough data to identify
	 * the application yet (no payload available yet).
	 */
	LI_UNKNOWN = 0U,
	/*
	 * Undefined means there's no matching application
	 * available in the library. This is needed to
	 * distinguish between unknown and known!
	 */
	LI_UNDEFINED,
	/*
	 * Applications are ordered by their likelyhood of
	 * appearance *and* how well the detection works.
	 * If an application is misbehaving, move it past
	 * the one that is being shadowed. There is no exact
	 * sience involved here -- it's only best practise.
	 */
	LI_HTTP,
	LI_RTSP,
	LI_POP3,
	LI_IMAP,
	LI_SMTP,
	LI_STUN,
	LI_PPTP,
	LI_TELNET,
	LI_DHCP,
	LI_SSH,
	LI_IRC,
	LI_FTP,
	LI_SIP,
	LI_DNS,
	LI_CVS,
	LI_BITTORRENT,
	LI_GNUTELLA,
	LI_NETBIOS,
	LI_RTCP,
	LI_RTP,
	LI_RIP,
	LI_BGP,
	LI_IKE,
	LI_DTLS,
	LI_TLS,
	LI_LDAP,
	LI_SNMP,
	LI_TFTP,
	LI_XMPP,
	LI_IMPP,
	LI_OPENVPN,
	LI_SYSLOG,
	LI_L2TP,
	LI_NTP,
	LI_NETFLOW,
	LI_RADIUS,
	/*
	 * Shallow applications are only detected by their
	 * respective IP type, but we want to make sure they
	 * are becoming real detections sooner or later!
	 */
	LI_OSPF,
	LI_ICMP,
	LI_IGMP,
};

unsigned int	 peak_li_test(const struct peak_packet *,
		     const unsigned int);
unsigned int	 peak_li_get(const struct peak_packet *);
const char	*peak_li_name(const unsigned int);
unsigned int	 peak_li_number(const char *);

static inline unsigned int
peak_li_merge(const uint16_t array[2])
{
	unsigned int ret = MAX(array[0], array[1]);

	if (unlikely(ret == LI_UNDEFINED &&
	    (array[0] == LI_UNKNOWN || array[1] == LI_UNKNOWN))) {
		/*
		 * If only one side is undefined, we don't want
		 * to announce a completely undefined protocol
		 * just yet, because it might change to something
		 * else.  Direct protocol detection is not affected.
		 */
		ret = LI_UNKNOWN;
	}

	return (ret);
}

#endif /* !PEAK_LI_H */
