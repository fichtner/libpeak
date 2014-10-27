/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
 * Copyright (c) 2013 Victor Pereira <victor@packetwerk.com>
 * Copyright (c) 2013 Masoud Chelongar <masoud@packetwerk.com>
 * Copyright (c) 2014 Thomas Siegmund <thomas@packetwerk.com>
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
#include <sys/socket.h>
#include <netinet/in.h>

#define LI_ISDIGIT(x)		(((x) >= '0') && ((x) <= '9'))

#define LI_STRLEN(str) ({						\
	const ssize_t _off = (ssize_t)(str) - (ssize_t)packet->app.raw;	\
	size_t _ret = 0;						\
	if (likely(_off >= 0 && _off < packet->app_len)) {		\
		while (packet->app.raw[_off + _ret] != '\0') {		\
			++_ret;						\
			if (unlikely(_off + _ret >= packet->app_len)) {	\
				_ret = 0;				\
				break;					\
			}						\
		}							\
	}								\
	_ret;								\
})

#define LI_EXCLUDE_PORT(prt) do {					\
	if (XOR(packet->flow_sport == (prt),				\
	    packet->flow_dport == (prt))) {				\
		return (0);						\
	}								\
} while (0)

#define LI_MATCH_PAYLOAD(keys, min, ret) do {				\
	unsigned int _i;						\
	if (packet->app_len < (min)) {					\
		/* safeguard: can't match less in payload */		\
		return (0);						\
	}								\
	for (_i = 0; _i < lengthof(keys); ++_i) {			\
		if (!memcmp(packet->app.raw, (keys)[_i], (min))) {	\
			return (ret);					\
		}							\
	}								\
} while (0)

#define LI_LIST_APP(number, name, p1, p2, pretty, desc, cat)			\
	{ peak_li_##name, { p1, p2 }, number, #name, pretty, desc, cat }

#define LI_LIST_IPTYPE(number, name, p1, p2, pretty, desc, cat)		\
	{ peak_li_iptype, { p1, p2 }, number, #name, pretty, desc, cat }

#define LI_DESCRIBE_APP(name)						\
static unsigned int							\
peak_li_##name(const struct peak_packet *packet)

#define LI_CALL_APP(name)	peak_li_##name(packet)

struct peak_lis {
	unsigned int (*function)(const struct peak_packet *);
	unsigned short proto[2];
	unsigned int number;
	const char *name;
	const char *pretty;
	const char *desc;
	const char *cat;
};

LI_DESCRIBE_APP(dns)
{
	/* TCP: padded with 2 bytes of length */
	const size_t padding =
	    (packet->net_type == IPPROTO_TCP) * sizeof(uint16_t);
	struct dns {
		uint16_t id;
		uint16_t flags;
		uint16_t question_count;
		uint16_t answer_count;
		uint16_t ns_count;
		uint16_t ar_count;
	} __packed *ptr = (void *)&packet->app.raw[padding];
	uint16_t decoded;

	if (packet->app_len < sizeof(struct dns) + padding) {
		return (0);
	}

	if (padding && be16dec(packet->app.raw) +
	    sizeof(uint16_t) < packet->app_len) {
		/* TCP: verify that length is somewhat correct */
		return (0);
	}

	/* verification pimped according to RFC 1035 */
	decoded = be16dec(&ptr->flags);
	if (decoded & 0x0070) {
		/* reserved for future use */
		return (0);
	}

	switch (decoded & 0x7800) {
	case (0 << 12):	/* a standard query (QUERY) */
	case (1 << 12):	/* an inverse query (IQUERY) */
	case (2 << 12):	/* a server status request (STATUS) */
		break;
	default:
		return (0);
	}

	if (decoded & 0x8000) {		/* response handling */
		switch (decoded & 0x000f) {
		case 0:	/* No error condition */
		case 1:	/* Format error */
		case 3:	/* Server failure */
		case 4:	/* Name error */
		case 5:	/* Refused */
			break;
		default:
			/* reserved for future use */
			return (0);
		}
	} else {			/* request handling */
		if (decoded & 0x000f) {
			/* can't have response code */
			return (0);
		}

		if (decoded & 0x0480) {
			/* AA and RA bits only for response */
			return (0);
		}

		if (!ptr->question_count) {
			/* there should be a question */
			return (0);
		}

		if (ptr->answer_count) {
			/* no answers yet */
			return (0);
		}
	}

	return (1);
}

LI_DESCRIBE_APP(dhcp)
{
	struct dhcp {
		uint8_t op;
		uint8_t htype;
		uint8_t hlen;
		uint8_t hops;
		uint32_t xid;
		uint16_t secs;
		uint16_t flags;
		uint32_t ciaddr;
		uint32_t yiaddr;
		uint32_t siaddr;
		uint32_t giaddr;
		uint8_t chaddr[16];
		uint8_t sname[64];
		uint8_t file[128];
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct dhcp)) {
		return (0);
	}

	switch (ptr->op) {
	case 1:	/* BOOTREQUEST */
	case 2: /* BOOTREPLY */
		break;
	default:
		return (0);
	}

	if (ptr->htype != 1 || ptr->hlen != 6) {
		/* Ethernet / MAC length is 6 bytes */
		return (0);
	}

	/*
	 * This should be obvious enough.  If not,
	 * record traces and read RFC 2131.
	 */

	return (1);
}

LI_DESCRIBE_APP(stun)
{
	static const char *strings[] = {
		"RSP/",
	};
	struct stun {
		uint16_t type;
		uint16_t length;
		uint32_t cookie;
		uint32_t id[3];
	} __packed *ptr = (void *)packet->app.raw;

	LI_MATCH_PAYLOAD(strings, 4, 1);

	if (packet->app_len < sizeof(struct stun)) {
		return (0);
	}

	if (be32dec(&ptr->cookie) != 0x2112A442) {
		return (0);
	}

	/*
	 * We could also check the type, but
	 * the magic value is kinda obvious...
	 */

	return (1);
}

LI_DESCRIBE_APP(cvs)
{
	static const char *strings[] = {
		/* client */
		"BEGI",
		/* server */
		"I LO",
		"I HA",
	};

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(rtcp)
{
	struct rtcp {
		uint8_t v_p_count;
		uint8_t type;
		uint16_t length;
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct rtcp)) {
		return (0);
	}

	if ((ptr->v_p_count >> 6) != 2) {
		return (0);
	}

	switch (ptr->type) {
	case 192:	/* FIR -- RFC 2032 */
	case 193:	/* NACK -- RFC 2032 */
	case 194:	/* SMPTETC -- RFC 5484 */
	case 195:	/* IJ -- RFC 5450 */
	case 200:	/* SR -- RFC 3550 */
	case 201:	/* RR -- RFC 3550 */
	case 202:	/* SDES -- RFC 3550 */
	case 203:	/* BYE -- RFC 3550 */
	case 204:	/* APP -- RFC 3550 */
	case 205:	/* RTPFB */
	case 206:	/* PSFB */
	case 207:	/* XR -- RFC 3611 */
	case 208:	/* AVB -- IEEE 1733 */
	case 209:	/* RSI -- RFC 5760 */
		break;
	default:
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(rtp)
{
	struct rtp {
		uint8_t v_p_x_cc;
		uint8_t m_pt;
		uint16_t sequence_number;
		uint32_t timestamp;
		uint32_t ssrc_id;
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct rtp)) {
		return (0);
	}

	if ((ptr->v_p_x_cc >> 6) != 2) {
		return (0);
	}

	if (!ptr->timestamp) {
		/* an educated guess */
		return (1);
	}

	switch (ptr->m_pt & 0x7f) {
	case 0:		/* PCMU */
	case 3:		/* GSM */
	case 4:		/* G723 */
	case 5:		/* DVI4 */
	case 6:		/* DVI4 */
	case 7:		/* LPC */
	case 8:		/* PCMA */
	case 9:		/* G722 */
	case 10:	/* L16 */
	case 11:	/* L16 */
	case 12:	/* QCELP */
	case 13:	/* CN */
	case 14:	/* MPA */
	case 15:	/* G728 */
	case 16:	/* DIV4 */
	case 17:	/* DIV4 */
	case 18:	/* G729 */
	case 25:	/* CelB */
	case 26:	/* JPEG */
	case 28:	/* nv */
	case 31:	/* H261 */
	case 32:	/* MPV */
	case 33:	/* MP2T */
	case 34:	/* H263 */
		break;
	default:
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(sip)
{
	static const char *strings[] = {
		/* response */
		"SIP/",
		/* RFC 3261 */
		"INVI",
		"ACK ",
		"BYE ",
		"CANC",
		"OPTI",
		"REGI",
		/* RFC 3262 */
		"PRAC",
		"SUBS",
		"NOTI",
		/* RFC 3903 */
		"PUBL",
		/* RFC 6086 */
		"INFO",
		/* RFC 3415 */
		"REFE",
		/* RFC 3428 */
		"MESS",
		/* RFC 3311 */
		"UPDA",
	};

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(ftp)
{
	static const char *strings[] = {
		/* server init */
		"220 ",
		"220-",
		/* client login */
		"USER",
	};

	LI_EXCLUDE_PORT(110);	/* pop3 */
	LI_EXCLUDE_PORT(25);	/* smtp */
	LI_EXCLUDE_PORT(465);	/* smtp */
	LI_EXCLUDE_PORT(587);	/* smtp */
	LI_EXCLUDE_PORT(194);	/* irc */
	LI_EXCLUDE_PORT(6667);	/* irc */

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(ssh)
{
	static const char *strings[] = {
		"SSH-",
	};

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(imap)
{
	static const char *strings[] = {
		"* OK",
	};

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(smtp)
{
	static const char *strings[] = {
		/* server greetings */
		"220 ",
		"220-",
		/* client hello */
		"HELO",
		"EHLO",
	};

	LI_EXCLUDE_PORT(21);	/* ftp */

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(pop3)
{
	static const char *strings[] = {
		/* server noise */
		"+OK ",
		/* client blabber */
		"USER",
		"CAPA",
		"AUTH",
	};

	LI_EXCLUDE_PORT(21);	/* ftp */
	LI_EXCLUDE_PORT(194);	/* irc */
	LI_EXCLUDE_PORT(6667);	/* irc */

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(telnet)
{
	unsigned int next = 3;	/* standard command */

	if (packet->app_len < 2) {
		/* minimum length is 2 bytes */
		return (0);
	}

	if (packet->app.raw[0] != 0xFF) {
		/* not a telnet standard command */
		return (0);
	}

	/* safe to pull second character, see first check above */
	switch (packet->app.raw[1]) {
	case 0xF0:
		next = 2;	/* suboption end (2 bytes) */
		break;
	case 0xFA:
		next = 4;	/* suboption start (4 bytes) */
		break;
	default:
		break;
	}

	if (packet->app_len > next && packet->app.raw[next] != 0xFF) {
		/* command sequence must repeat */
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(rip)
{
	struct rip {
		/* header */
		uint8_t command;
		uint8_t version;
		uint16_t zero;
		/* 1st entry */
		uint16_t family_id;
		uint16_t route_tag;	/* zero for v1 */
		uint32_t ipv4_addr;
		uint32_t subnet_mask;	/* zero for v1 */
		uint32_t next_hop;	/* zero for v1 */
		uint32_t metric;
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct rip)) {
		return (0);
	}

	if (ptr->zero) {
		return (0);
	}

	switch (ptr->version) {
	case 1:
	case 2:
		break;
	default:
		return (0);
	}

	switch (ptr->command) {
	case 1:	/* request */
	case 2:	/* response */
		break;
	default:
		return (0);
	}

	/*
	 * More verification could be added here,
	 * especially for RIPv1, but then again
	 * the protocol is pretty distinctive and
	 * nobody is going to use RIPv1 so badly.
	 * RFC 2453 has quite a few hints on how
	 * to proceed if needed.  Have fun...
	 */

	return (1);
}

LI_DESCRIBE_APP(dtls)
{
	struct dtls {
		uint8_t content_type;
		uint16_t version;
		uint16_t epoch;
		uint16_t sequence_number[3];
		uint16_t data_length;
	} __packed *ptr = (void *)packet->app.raw;
	uint16_t decoded;

	if (packet->app_len < sizeof(struct dtls)) {
		return (0);
	}

	decoded = be16dec(&ptr->data_length);

	if (!decoded || decoded > 0x4000) {
		/* never zero or higher than 2^14 */
		return (0);
	}

	if (be16dec(&ptr->version) != 0xFEFF) {
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(tls)
{
	struct tls {
		uint8_t record_type;
		uint16_t version;
		uint16_t data_length;
	} __packed *ptr = (void *)packet->app.raw;
	uint16_t decoded;

	if (packet->app_len < sizeof(struct tls)) {
		return (0);
	}

	decoded = be16dec(&ptr->data_length);

	if (!decoded || decoded > 0x4000) {
		/* no empty records possible, also <= 2^14 */
		return (0);
	}

	switch (ptr->record_type) {
	case 20:	/* change_cipher_spec */
	case 21:	/* alert */
	case 22:	/* handshake */
	case 23:	/* application_data */
		break;
	default:
		return (0);
	}

	switch (be16dec(&ptr->version)) {
	case 0x0300:	/* SSL 3.0 */
	case 0x0301:	/* TLS 1.0 */
	case 0x0302:	/* TLS 1.1 */
	case 0x0303:	/* TLS 1.2 */
		break;
	default:
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(ike)
{
	struct ike {
		uint64_t initiator_cookie;
		uint64_t responder_cookie;
		uint8_t next_payload;
		uint8_t version;
		uint8_t exchange_type;
		uint8_t flags;
		uint32_t message_id;
		uint32_t length;
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct ike)) {
		return (0);
	}

	if (!ptr->initiator_cookie) {
		/* never zero according to RFC 5996 */
		return (0);
	}

	switch (ptr->version >> 4) {
	case 1:	/* ISAKMP */
		if (ptr->next_payload < 1 ||
		    ptr->next_payload > 13) {
			return (0);
		}
		if (ptr->exchange_type < 1 ||
		    ptr->exchange_type > 5) {
			return (0);
		}
		break;
	case 2:	/* IKE(v2) */
		/*
		 * http://www.iana.org/assignments/ikev2-parameters/
		 * ikev2-parameters.txt
		 */
		if (ptr->next_payload < 33 ||
		    ptr->next_payload > 49) {
			return (0);
		}
		if (ptr->exchange_type < 34 ||
		    ptr->exchange_type > 38) {
			return (0);
		}
		break;
	default:
		return (0);
	}

	if (be32dec(&ptr->length) != packet->app_len) {
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(bgp)
{
	struct bgp {
		uint32_t marker[4];
		uint16_t msg_length;
		uint8_t msg_type;
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct bgp)) {
		return (0);
	}

	if (memcmp(ptr->marker, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
	    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", sizeof(ptr->marker))) {
		/* marker is filled with ones */
		return (0);
	}

	if (be16dec(&ptr->msg_length) < sizeof(struct bgp)) {
		/* total length must include header length */
		return (0);
	}

	switch (ptr->msg_type) {
	case 1:		/* Open		RFC 4271 */
	case 2:		/* Update	RFC 4271 */
	case 3:		/* Notification	RFC 4271 */
	case 4:		/* KeepAlive	RFC 4271 */
	case 5:		/* RouteRefresh	RFC 2918 */
		break;
	default:	/* not a known type */
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(http)
{
	static const char *strings[] = {
		/* general response */
		"HTTP",
		/* vanilla requests */
		"GET ",
		"HEAD",
		"POST",
		"PUT ",
		"DELE",
		"TRAC",
		"CONN",
		"OPTI",
		/* WebDAV */
		"PROP",
		"MKCO",
		"COPY",
		"MOVE",
		"LOCK",
		"UNLO",
		/* Ntrip */
		"SOUR",
	};

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(rtsp)
{
	static const char *strings[] = {
		/* RFC 2326 */
		"RTSP",
		"DESC",
		"ANNO",
		"GET_",
		"OPTI",
		"PAUS",
		"PLAY",
		"RECO",
		"REDI",
		"SETU",
		"SET_",
		"TEAR",
	};

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(netflow_v1)
{
	struct netflow_v1_record {
		uint32_t src_addr;
		uint32_t dst_addr;
		uint32_t next_hop;
		uint16_t input;
		uint16_t output;
		uint32_t d_packets;
		uint32_t d_octet;
		uint32_t first;
		uint32_t last;
		uint16_t src_port;
		uint16_t dst_port;
		uint16_t pad1;
		uint8_t port;
		uint8_t tos;
		uint8_t flags;
		uint8_t pad2[3];
		uint32_t reserved;
	} __packed;
	struct netflow_v1_header {
		uint16_t version;
		uint16_t count;
		uint32_t sys_uptime;
		uint32_t unix_secs;
		uint32_t unix_nsecs;
		struct netflow_v1_record fl_rec[];
	} __packed *ptr = (void *)packet->app.raw;
	uint16_t decoded;

	if (packet->app_len < sizeof(struct netflow_v1_header) +
	    sizeof(struct netflow_v1_record)) {
		/* at least a header and one record */
		return (0);
	}
	decoded = be16dec(&ptr->count);
	if (!decoded || decoded > 24) {
		/* count is between 1 and 24 */
		return (0);
	}

	if (ptr->fl_rec[0].pad1) {
		/* must be zero */
		return (0);
	}

	if (memcmp(ptr->fl_rec[0].pad2, "\0\0\0",
	    sizeof(ptr->fl_rec[0].pad2))) {
		/* must be zero */
		return (0);
	}

	if (!ptr->fl_rec[0].src_addr || !ptr->fl_rec[0].dst_addr) {
		/* IP addresses can't be zero */
		return (0);
	}

	if (packet->app_len != sizeof(struct netflow_v1_header) +
	    decoded * sizeof(struct netflow_v1_record)) {
		/* in UDP packet length corresponds to actual length */
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(netflow_v5)
{
	struct netflow_v5_record {
		uint32_t src_addr;
		uint32_t dst_addr;
		uint32_t next_hop;
		uint16_t input;
		uint16_t output;
		uint32_t d_packets;
		uint32_t d_octet;
		uint32_t first;
		uint32_t last;
		uint16_t src_port;
		uint16_t dst_port;
		uint8_t pad1;
		uint8_t tcp_flag;
		uint8_t port;
		uint8_t tos;
		uint16_t src_as;
		uint16_t dst_as;
		uint8_t src_mask;
		uint8_t dst_mask;
		uint16_t pad2;
	} __packed;
	struct netflow_v5_header {
		uint16_t version;
		uint16_t count;
		uint32_t sys_uptime;
		uint32_t unix_secs;
		uint32_t unix_nsecs;
		uint32_t flow_sequence;
		uint8_t engine_type;
		uint8_t engine_id;
		uint16_t sampling_interval;
		struct netflow_v5_record fl_rec[];
	} __packed *ptr = (void *)packet->app.raw;
	uint16_t decoded;

	if (packet->app_len < sizeof(struct netflow_v5_header) +
	    sizeof(struct netflow_v5_record)) {
		/* at least a header and one record */
		return (0);
	}

	decoded = be16dec(&ptr->count);
	if (!decoded || decoded > 30) {
		/* only between 1 and 30 records */
		return (0);
	}

	if (ptr->fl_rec[0].pad1 || ptr->fl_rec[0].pad2) {
		/* must be zero */
		return (0);
	}

	if (!ptr->fl_rec[0].src_addr || !ptr->fl_rec[0].dst_addr) {
		/* IP addresses can't be zero */
		return (0);
	}

	if (packet->app_len != sizeof(struct netflow_v5_header) +
	    decoded * sizeof(struct netflow_v5_record)) {
		/* UDP length must match */
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(netflow)
{
	uint16_t *version = (void *)packet->app.raw;

	if (packet->app_len < sizeof(*version)) {
		return (0);
	}

	switch (be16dec(version)) {
	case 1:
		return (LI_CALL_APP(netflow_v1));
	case 5:
		return (LI_CALL_APP(netflow_v5));
	default:
		break;
	}

	return (0);
}

LI_DESCRIBE_APP(radius)
{
	/* RFC 2865 */
	struct radius {
		uint8_t code;
		uint8_t identifier;
		uint16_t length;
		uint8_t authenticator[16];
	} __packed *ptr = (void *)packet->app.raw;
	uint16_t decoded;

	if (packet->app_len < sizeof(struct radius)) {
		return (0);
	}

	decoded = be16dec(&ptr->length);
	if (decoded < 20  || decoded > 4096) {
		/* size of packet should be between 20 and 4096 */
		return (0);
	}

	if (packet->app_len != decoded) {
		return (0);
	}

	switch (ptr->code) {
	case 1:		/* Access-Request */
	case 2:		/* Accept-Request */
	case 3:		/* Reject-Request */
	case 4:		/* Accounting-Requst */
	case 5:		/* Accounting-Response */
	case 11:	/* Access-Challenge */
	case 12:	/* Status-Server (experimental) */
	case 13:	/* Status-Client (experimental) */
		break;
	default:
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(bittorrent)
{
	static const char *strings[] = {
		"\x13""Bit",
	};

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(gnutella)
{
	static const char *strings[] = {
		"GNUT",
		"GIV ",
	};

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(netbios_name_service)
{
	/*
	 * RFC 1002 states the following:
	 * "If Name Service packets are sent over a TCP
	 *  connection they are preceded by a 16 bit
	 *  unsigned integer representing the length of
	 *  the Name service packet."
	 */
	const size_t padding =
	    (packet->net_type == IPPROTO_TCP) * sizeof(uint16_t);
	struct netbios {
		uint16_t name_trn_id;
		uint16_t opcode_flags_rcode;
		uint16_t qdcount;
		uint16_t ancount;
		uint16_t nscount;
		uint16_t arcount;
	} __packed *ptr = (void *)&packet->app.raw[padding];
	uint16_t decoded;

	if (packet->app_len < sizeof(struct netbios) + padding) {
		return (0);
	}

	decoded = be16dec(&ptr->opcode_flags_rcode);

	if (decoded & 0x0060) {
		/* both bits are zero */
		return (0);
	}

	switch (decoded & 0x000F) {
	case 0x0:	/* no error */
	case 0x1:	/* FMT_ERR */
	case 0x2:	/* SRV_ERR */
	case 0x3:	/* NAM_ERR */
	case 0x4:	/* IMP_ERR */
	case 0x5:	/* RFS_ERR */
	case 0x6:	/* ACT_ERR */
	case 0x7:	/* CFT_ERR */
		break;
	default:
		/* unknown return code */
		return (0);
	}

	switch (decoded & 0x7800) {
	case (0 << 11):	/* query */
	case (5 << 11): /* registration */
	case (6 << 11): /* release */
	case (7 << 11):	/* WACK */
	case (8 << 11): /* refresh */
		break;
	default:
		/* unknown op code */
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(tftp)
{
	struct tftp {
		uint16_t opcode;
		union {
			uint16_t block_or_error_code;
			char file[2];
		} u;
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct tftp)) {
		return (0);
	}

	switch (be16dec(&ptr->opcode)) {
	case 1:		/* Read request (RRQ) */
	case 2: {	/* Write request (WRQ) */
		const size_t file_len = LI_STRLEN(ptr->u.file);
		if (!file_len) {
			/* no terminated file name string in packet */
			return (0);
		}

		if (!LI_STRLEN(ptr->u.file + file_len + 1)) {
			/* no terminated mode string in packet */
			return (0);
		}

		switch (ptr->u.file[file_len + 1]) {
		case 'O': case 'o':	/* no case: "octet" */
		case 'N': case 'n':	/* no case: "netascii */
		case 'M': case 'm':	/* no case: "mail" */
			break;
		default:
			return (0);
		}

		break;
	}
	case 3:		/* Data (DATA) */
		if (!ptr->u.block_or_error_code) {
			/* block numbers are always set */
			return (0);
		}
		if (packet->app_len > sizeof(struct tftp) + 512) {
			/* maximum data length is 512 bytes */
			return (0);
		}
		/*
		 * Unfortunately, data packets may hold data from as
		 * little as zero to 512 bytes.  No more checking.  :(
		 */
		break;
	case 4:		/* Acknowledgement (ACK) */
		if (packet->app_len > sizeof(struct tftp)) {
			/* only minimum length */
			return (0);
		}
		/*
		 * Unfortunately, write requests are acknowledged with
		 * a zero, so there's no room for sanity checking.
		 */
		break;
	case 5:		/* Error (ERROR) */
		switch (be16dec(&ptr->u.block_or_error_code)) {
		case 0:	/* Not defined, see error message (if any). */
		case 1:	/* File not found. */
		case 2:	/* Access violation. */
		case 3: /* Disk full or allocation exceeded. */
		case 4:	/* Illegal TFTP operation. */
		case 5:	/* Unknown transfer ID. */
		case 6:	/* File already exists. */
		case 7:	/* No such user. */
			break;
		default:
			return (0);
		}
		break;
	default:
		return (0);
	}

	return (1);
}
LI_DESCRIBE_APP(netbios_session_service)
{
	struct netbios {
		uint8_t type;
		uint8_t flags;
		uint16_t length;
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->net_type != IPPROTO_TCP) {
		/*
		 * We run NetBIOS detection on TCP and UDP,
		 * but RFC 1002 tells us the Session Service
		 * only works on TCP, so bail here...
		 */
		return (0);
	}

	if (packet->app_len < sizeof(struct netbios)) {
		return (0);
	}

	if (ptr->flags & 0xFE) {
		/* all of these are zero */
		return (0);
	}

	switch (ptr->type) {
	case 0x00:	/* SESSION MESSAGE */
	case 0x81:	/* SESSION REQUEST */
	case 0x82:	/* POSITIVE SESSION RESPONSE */
	case 0x83:	/* NEGATIVE SESSION RESPONSE */
	case 0x84:	/* RETARGET SESSION RESPONSE */
	case 0x85:	/* SESSION KEEP ALIVE */
		break;
	default:
		return (0);
	}

	/* can't check length, it doesn't tell us much :( */

	return (1);
}

LI_DESCRIBE_APP(netbios_datagram_service)
{
	struct netbios {
		uint8_t msg_type;
		uint8_t flags;
		uint16_t dmg_id;
		uint32_t source_ip;
		uint16_t source_port;
		uint16_t dmg_length;
		uint16_t packet_offset;
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->net_type != IPPROTO_UDP) {
		/*
		 * We run NetBIOS detection on TCP and UDP,
		 * but RFC 1002 _doesn't_ tell us the Datagram
		 * Service only works on UDP, so we just assume
		 * it!
		 */
		return (0);
	}

	if (packet->app_len < sizeof(struct netbios)) {
		return (0);
	}

	if (ptr->flags & 0xF0) {
		/* unused flags are zero! */
		return (0);
	}

	if (!ptr->source_ip || !ptr->source_port) {
		/* I'm assuming these can't be zero */
		return (0);
	}

	switch (ptr->msg_type) {
	case 0x10:	/* DIRECT_UNIQUE DATAGRAM */
	case 0x11:	/* DIRECT_GROUP DATAGRAM */
	case 0x12:	/* BROADCAST DATAGRAM */
	case 0x13:	/* DATAGRAM ERROR */
	case 0x14:	/* DATAGRAM QUERY REQUEST */
	case 0x15:	/* DATAGRAM POSITIVE QUERY RESPONSE */
	case 0x16:	/* DATAGRAM NEGATIVE QUERY RESPONSE */
		break;
	default:
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(netbios)
{
	if (LI_CALL_APP(netbios_datagram_service) ||
	    LI_CALL_APP(netbios_session_service) ||
	    LI_CALL_APP(netbios_name_service)) {
		return (1);
	}

	return (0);
}

LI_DESCRIBE_APP(pptp_raw)
{
	struct pptp {
		uint16_t length;
		uint16_t type;
		uint32_t cookie;
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct pptp)) {
		return (0);
	}

	if (be32dec(&ptr->cookie) != 0x1A2B3C4D) {
		/* magic cookie -- yummy! */
		return (0);
	}

	if (be16dec(&ptr->length) < sizeof(struct pptp)) {
		/* length includes header */
		return (0);
	}

	switch (be16dec(&ptr->type)) {
	case 1:	/* control message */
	case 2:	/* management message */
		break;
	default:
		return (0);
	}

	/*
	 * A control message may follow.  RFC 2637 has some
	 * fascinating insights on how this continues to be
	 * an interesting opportunity to verify more bytes
	 * into the payload.  However, we don't really care.
	 */

	return (1);
}

LI_DESCRIBE_APP(l2tp)
{
	struct l2tp {
		uint8_t flags;
		uint8_t version;
		uint16_t length;
		uint16_t tunnel_id;
		uint16_t session_id;
		uint16_t ns;
		uint16_t nr;
	} __packed *ptr = (void *)packet->app.raw;

	if ((packet->app_len) < (sizeof(struct l2tp))) {
		return (0);
	}

	switch (ptr->version) {
	case 2:		/* RFC 2661 */
	case 3:		/* RFC 3931 */
		break;
	default:
		return (0);
	}

	if (!(ptr->flags & 0x80)) {
		/*
		 * RFC 3931 doesn't mention a type bit,
		 * so it's safe to assume the detection
		 * of data messages is next to impossible.
		 * Instead, we keep the matching consistent
		 * by not even detecting data messages as
		 * per RFC 2661.
		 */
		return (0);
	}

	if (!(ptr->flags & 0x78)) {
		/* length and sequence bit must be set */
		return (0);
	}

	if (ptr->flags & 0x37) {
		/* must always be zero */
		return (0);
	}

	if (packet->app_len != be16dec(&ptr->length)) {
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(ntp)
{
	struct ntp {
		uint8_t flags;
		uint8_t stratum;
		int8_t poll_interval;
		int8_t precision;
		int32_t root_delay;
		int32_t root_dispersion;
		int32_t reference_clock_id;
		uint64_t reference_timestamp;
		uint64_t originate_timestamp;
		uint64_t receive_timestamp;
		uint64_t transmit_timestamp;
	} __packed *ptr = (void *)packet->app.raw;
	struct ntp_v2 {
		struct ntp ntp;
		uint8_t auth[12];
	} __packed;
	struct ntp_v4 {
		struct ntp ntp;
		uint32_t key_id;
		uint8_t dgst[16];
	} __packed;

	if (packet->app_len < sizeof(struct ntp)) {
		return (0);
	}

	switch (ptr->flags & 0x38) {
	case (1 << 3):  /* RFC 1059 */
		if ((packet->app_len) != sizeof(struct ntp)) {
			/* fixed header length */
			return (0);
		}
		if (ptr->flags & 0x05) {
			/* reserved and zeroed */
			return (0);
		}
		break;
	case (2 << 3):  /* RFC 1119 */
	case (3 << 3):  /* RFC 1305 */
		switch (packet->app_len) {
		case sizeof(struct ntp):
		case sizeof(struct ntp_v2):
			break;
		default:
			return (0);
		}
		if (!(ptr->flags & 0x05)) {
			/* zero is reserved */
			return (0);
		}
		break;
	case (4 << 3):	/* RFC 5905 */
		switch (packet->app_len) {
		case sizeof(struct ntp):
		case sizeof(struct ntp_v4):
			break;
		default:
			return (0);
		}
		if (!(ptr->flags & 0x05)) {
			/* zero is reserved */
			return (0);
		}
		break;
	default:
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(pptp_gre)
{
	/*
	 * GRE for PPTP is so distinctive that we don't have
	 * to bother checking for further headers.
	 */
	struct gre {
		uint8_t CRKSs_recur;
		uint8_t A_flags_version;
		uint16_t protocol;
		uint16_t payload_length;
		uint16_t call_id;
	} __packed *ptr = (void *)packet->flow.raw;

	if (packet->flow_len < sizeof(struct gre)) {
		return (0);
	}

	if (be16dec(&ptr->protocol) != 0x880B) {
		/* byteswapped check */
		return (0);
	}

	if (ptr->CRKSs_recur & 0xCF) {
		/* these bits are always zero */
		return (0);
	}

	if (!(ptr->CRKSs_recur & 0x20)) {
		/* key always present */
		return (0);
	}

	if ((ptr->A_flags_version & 0x07) != 1) {
		/* version is set to 1 */
		return (0);
	}

	if ((ptr->A_flags_version & 0x78)) {
		/* flags always zero */
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(pptp)
{
	if (packet->net_type == IPPROTO_GRE) {
		return (LI_CALL_APP(pptp_gre));
	}

	return (LI_CALL_APP(pptp_raw));
}

LI_DESCRIBE_APP(ldap)
{
	/*
	 * LDAP uses BER encoding, so the detection needs
	 * to check this first.  Then we can move on to
	 * request/response code validation.  For more on
	 * the topic see RFC 4511, but don't say I didn't
	 * warn you -- that stuff is weird!
	 */
	struct ldap {
		uint8_t identifier_type;
		uint8_t identifier_length;
		uint32_t identifier_value;
		uint8_t message_type;
		uint8_t message_length;
		uint8_t message_value[];
#define op_type	message_value[ptr->message_length]
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct ldap)) {
		return (0);
	}

	if (ptr->identifier_type != 0x30) {
		/*
		 * Only detect the structure tag.  This makes sure
		 * that we don't have to deal with simple types,
		 * because they won't creep up at the start of a
		 * connection (at least a control message must be
		 * sent).
		 */
		return (0);
	}

	if (!(ptr->identifier_length & 0x80) ||
	    (ptr->identifier_length & 0x7F) !=
	    sizeof(ptr->identifier_value)) {
		/* not a long form or not 4 bytes long */
		return (0);
	}

	/* XXX what happens if short form is used? */

	if (ptr->message_type != 0x02 || !ptr->message_length) {
		/* INTEGER only */
		return (0);
	}

	if (packet->app_len < sizeof(struct ldap) + ptr->message_length) {
		/* No more data. */
		return (0);
	}

	if ((ptr->op_type & 0xE0) != 0x60) {
		/* verify application & non-primitive bits */
		return (0);
	}

	switch (ptr->op_type & 0x1F) {
	case 0:		/* LDAP_REQ_BIND */
	case 1:		/* LDAP_RES_BIND */
	case 2:		/* LDAP_REQ_UNBIND_30 */
	case 3:		/* LDAP_REQ_SEARCH */
	case 4:		/* LDAP_RES_SEARCH_ENTRY */
	case 5:		/* LDAP_RES_SEARCH_RESULT */
	case 6:		/* LDAP_REQ_MODIFY */
	case 7:		/* LDAP_RES_MODIFY */
	case 8:		/* LDAP_REQ_ADD */
	case 9:		/* LDAP_RES_ADD */
	case 10:	/* LDAP_DELETE_30 */
	case 11:	/* LDAP_DELETE */
	case 12:	/* LDAP_REQ_MODRDN */
	case 13:	/* LDAP_SEA_MODRDN */
	case 14:	/* LDAP_REQ_COMPARE */
	case 15:	/* LDAP_RES_COMPARE */
	case 16:	/* LDAP_REQ_ABANDON_30 */
	case 19:	/* LDAP_RES_SEARCH_REFERENCE */
	case 23:	/* LDAP_REQ_EXTENDED */
	case 24:	/* LDAP_RES_EXTENDED */
		break;
	default:
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(snmp)
{
	/*
	 * SNMP uses the same encoding as LDAP (BER), so
	 * the code looks a lot alike.  Also see RFC 1157.
	 */
	struct snmp {
		uint8_t identifier_type;
		uint8_t identifier_length;
		uint8_t version_type;
		uint8_t version_length;
		uint8_t version_value;
		uint8_t community_type;
		uint8_t community_length;
		uint8_t community_value[];
#define pdu_type	community_value[ptr->community_length]
#define pdu_length	community_value[ptr->community_length + 1]
#define pdu_value	community_value[ptr->community_length + 2]
	} __packed *ptr = (void *)packet->app.raw;

	if (packet->app_len < sizeof(struct snmp)) {
		return (0);
	}

	if (ptr->identifier_type != 0x30) {
		/* compound structure only */
		return (0);
	}

	if (ptr->version_type != 0x02) {
		/* version type is INTEGER */
		return (0);
	}

	if (ptr->version_length != 0x01) {
		/* one byte for version */
		return (0);
	}

	switch (ptr->version_value) {
	case 0:	/* v1 */
	case 1:	/* v2 */
	case 3:	/* v3 */
		break;
	default:
		return (0);
	}

	if (ptr->community_type != 0x04) {
		/* community type is OCTET STRING */
		return (0);
	}

	if (packet->app_len < sizeof(struct snmp) +
	    ptr->community_length + 2) {
		/*
		 * Can we peek beyond the community string
		 * into the PDU stuff?
		 */
		return (0);
	}

	switch (ptr->pdu_value) {
	case 0:	/* GETREQ */
	case 1:	/* GETNEXTREQ */
	case 2:	/* GETRESP */
	case 3:	/* SETREQ */
	case 4:	/* (it's a) TRAP */
	/* SNMPv2 */
	case 5:	/* GETBULKREQ */
	case 6:	/* INFORMREQ */
	case 7: /* TRAPV2 */
	case 8:	/* REPORT */
		break;
	default:
		return (0);
	};

	return (1);
}

LI_DESCRIBE_APP(impp)
{
	struct impp {
		uint8_t start_byte;
		uint8_t channel;
		uint16_t version;
	} __packed *ptr = (void *)packet->app.raw;

	/*
	 * Reference material for IMPP taken from:
	 *
	 *   https://www.trillian.im/impp/
	 */

	if (packet->app_len < sizeof(struct impp)) {
		return (0);
	}

	if (ptr->start_byte != 0x6F) {
		return (0);
	}

	switch (ptr->channel) {
	case 0x01:	/* version channel */
	case 0x02:	/* TLV channel */
		break;
	default:
		return (0);
	}

	/*
	 * Scott Werndorfer on June 25, 2013, answering
	 * the question of which versions of IMPP are
	 * acutally used:
	 *
	 *   "Probably 6-8 right now. 10 is on the way,
	 *    9 was short-lived internally.  Trillian 4.2
	 *    for Windows will continue to speak an older
	 *    protocol though; generally there will be
	 *    nothing older than 6 (as no servers support
	 *    it anymore and we've upgraded all the clients)."
	 */

	switch (be16dec(&ptr->version)) {
	case 6:
	case 7:
	case 8:
	case 10:
		break;
	default:
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(xmpp)
{
	static const char *strings[] = {
		"<?xm",
		"<str",
		"<pre",
	};

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(openvpn)
{
	const size_t padding =
	    (packet->net_type == IPPROTO_TCP) * sizeof(uint16_t);
	struct openvpn {
		uint8_t opcode;
		uint8_t session_id[8];
	} __packed *ptr = (void *)&packet->app.raw[padding];

	if (packet->app_len < sizeof(struct openvpn) + padding) {
		return (0);
	}

	if (padding && be16dec(packet->app.raw) +
	    sizeof(uint16_t) < packet->app_len) {
		/*
		 * This check considers one OpenVPN message being
		 * split into multiple segments, but may also
		 * fail when TCP decides to send two messages
		 * at once.  If that's the case, the check needs
		 * to be ditched...
		 */
		return (0);
	}

	switch (ptr->opcode) {
#ifdef notyet
	case (1 << 3):	/* P_CONTROL_HARD_RESET_CLIENT_V1 */
	case (2 << 3):	/* P_CONTROL_HARD_RESET_SERVER_V1 */
	case (3 << 3):	/* P_CONTROL_SOFT_RESET_V1 */
	case (6 << 3):	/* P_DATA_V1 */
#endif /* notyet */
	case (4 << 3):	/* P_CONTROL_V1 */
	case (5 << 3):	/* P_ACK_V1 */
	case (7 << 3):	/* P_CONTROL_HARD_RESET_CLIENT_V2 */
	case (8 << 3):	/* P_CONTROL_HARD_RESET_SERVER_V2 */
		break;
	default:
		return (0);
	}

	/* XXX one byte for detection is unstable */

	return (1);
}

LI_DESCRIBE_APP(irc)
{
	static const char *strings[] = {
		"NICK",
		"PASS",
		"USER",
		":irc",
		":loc",
		"WHO ",
		"WHOI",
		"PING",
		"CAP ",
		"JOIN",
		"PRIV",
	};

	LI_EXCLUDE_PORT(110);	/* pop3 */
	LI_EXCLUDE_PORT(21);	/* ftp */

	LI_MATCH_PAYLOAD(strings, 4, 1);

	return (0);
}

LI_DESCRIBE_APP(syslog)
{
	/* match "<x>" or "<xy", see RFC 3164 */

	if (packet->app_len < 3) {
		return (0);
	}

	if (packet->app.raw[0] != '<') {
		return (0);
	}

	if (!LI_ISDIGIT(packet->app.raw[1])) {
		return (0);
	}

	if (!LI_ISDIGIT(packet->app.raw[2]) &&
	    packet->app.raw[2] != '>') {
		return (0);
	}

	return (1);
}

LI_DESCRIBE_APP(iptype)
{
	(void)packet;
	return (1);
}

static const struct peak_lis apps[] = {
	/* shallow protocols first (match by IP type) */
	LI_LIST_IPTYPE(LI_ICMP, icmp, IPPROTO_ICMP, IPPROTO_ICMPV6, "Internet Control Message Protocol", "ICMP is one of the core protocols of the Internet Protocol Suite. It is chiefly used by the operating systems of networked computers to send error messages indicating, for instance, that a requested service is not available or that a host or router could not be reached. ICMP can also be used to relay query messages.", "Network Monitoring"),
	LI_LIST_IPTYPE(LI_IGMP, igmp, IPPROTO_IGMP, IPPROTO_MAX, "Internet Group Management Protocol", "IGMP is a communications protocol used by hosts and adjacent routers on IP networks to establish multicast group memberships.", "Networking"),
	LI_LIST_IPTYPE(LI_OSPF, ospf, IPPROTO_OSPFIGP, IPPROTO_MAX, "Open Shortest Path First", "OSPF is an interior gateway routing protocol developed for IP networks based on the shortest path first or link-state algorithm.", "Networking"),
	LI_LIST_IPTYPE(LI_L2TP, l2tp, IPPROTO_L2TP, IPPROTO_MAX, "Layer 2 Tunneling Protocol", "L2TP is a tunneling protocol used to support virtual private networks (VPNs). It does not provide any encryption or confidentiality by itself; it relies on an encryption protocol that it passes within the tunnel to provide privacy.", "VPN and Tunneling"),
	/* real protocols follow now */
	LI_LIST_APP(LI_PPTP, pptp, IPPROTO_TCP, IPPROTO_GRE, "Point-to-Point Tunneling Protocol", "PPTP is a method for implementing virtual private networks. It uses a control channel over TCP and a GRE tunnel operating to encapsulate PPP packets.", "VPN and Tunneling"),
	LI_LIST_APP(LI_HTTP, http, IPPROTO_TCP, IPPROTO_MAX, "HyperText Transfer Protocol", "HTTP is the principal transport protocol for the World Wide Web.", "Web Services"),
	LI_LIST_APP(LI_RTSP, rtsp, IPPROTO_TCP, IPPROTO_UDP, "Real Time Streaming Protocol", "RTSP is used for establishing and controlling media sessions between end points.", "Streaming Media"),
	LI_LIST_APP(LI_POP3, pop3, IPPROTO_TCP, IPPROTO_MAX, "Post Office Protocol", "POP is a protocol used by local e-mail clients to retrieve e-mail from a remote server.", "Mail"),
	LI_LIST_APP(LI_IMAP, imap, IPPROTO_TCP, IPPROTO_MAX, "Internet Message Access Protocol", "IMAP is an Internet standard protocol for accessing email on a remote server.", "Mail"),
	LI_LIST_APP(LI_SMTP, smtp, IPPROTO_TCP, IPPROTO_MAX, "Simple Mail Transfer Protocol", "SMTP is an Internet standard for electronic mail (e-mail) transmission across Internet Protocol (IP) networks.", "Mail"),
	LI_LIST_APP(LI_FTP, ftp, IPPROTO_TCP, IPPROTO_MAX, "File Transfer Protocol", "FTP is used to transfer files from a file server to a local machine.", "File Transfer"),
	LI_LIST_APP(LI_CVS, cvs, IPPROTO_TCP, IPPROTO_MAX, "Concurrent Versions System", "CVS is a client-server free software revision control system in the field of software development.", "Revision Control"),
	LI_LIST_APP(LI_SSH, ssh, IPPROTO_TCP, IPPROTO_MAX, "Secure Shell", "SSH is a network protocol that allows data to be exchanged using a secure channel between two networked devices.", "Remote Access"),
	LI_LIST_APP(LI_IRC, irc, IPPROTO_TCP, IPPROTO_MAX, "Internet Relay Chat", "IRC is a popular form of real-time Internet text messaging.", "Messaging"),
	LI_LIST_APP(LI_STUN, stun, IPPROTO_UDP, IPPROTO_TCP, "Session Traversal Utilities for NAT", "STUN is used in NAT traversal for applications with real-time voice, video, messaging, and other interactive communications.", "Networking"),
	LI_LIST_APP(LI_SIP, sip, IPPROTO_UDP, IPPROTO_TCP, "Session Initiation Protocol", "SIP is a common control protocol for setting up and controlling voice and video calls.", "Streaming Media"),
	LI_LIST_APP(LI_RIP, rip, IPPROTO_UDP, IPPROTO_MAX, "Routing Information Protocol", "RIP is a dynamic routing protocol.", "Networking"),
	LI_LIST_APP(LI_RADIUS, radius, IPPROTO_UDP, IPPROTO_MAX, "Remote Authentication Dial In User Service", "RADIUS is a networking protocol that provides centralized Authentication, Authorization, and Accounting (AAA) management for computers to connect and use a network service.", "Networking"),
	LI_LIST_APP(LI_BGP, bgp, IPPROTO_TCP, IPPROTO_MAX, "Border Gateway Protocol", "BGP is the protocol backing the core routing decisions on the Internet.", "Networking"),
	LI_LIST_APP(LI_IKE, ike, IPPROTO_UDP, IPPROTO_MAX, "Internet Key Exchange", "IKE is the protocol used to set up a security association (SA) in the IPsec protocol suite.", "VPN and Tunneling"),
	LI_LIST_APP(LI_NETFLOW, netflow, IPPROTO_UDP, IPPROTO_MAX, "NetFlow", "NetFlow is a network reporting protocol developed by Cisco.", "Networking"),
	LI_LIST_APP(LI_TFTP, tftp, IPPROTO_UDP, IPPROTO_MAX, "Trivial File Transfer Protocol", "TFTP is a file transfer protocol, with the functionality of a very basic form of FTP (File Transfer Protocol).", "File Transfer"),
	LI_LIST_APP(LI_DHCP, dhcp, IPPROTO_UDP, IPPROTO_MAX, "Dynamic Host Configuration Protocol", "DHCP is an auto configuration protocol used for assigning IP addresses.", "Networking"),
	LI_LIST_APP(LI_DTLS, dtls, IPPROTO_UDP, IPPROTO_MAX, "Datagram Transport Layer Security", "DTLS is a protocol that provides communication privacy for datagram protocols.", "Encryption"),
	LI_LIST_APP(LI_TLS, tls, IPPROTO_TCP, IPPROTO_MAX, "Transport Layer Security", "TLS is a cryptographic protocol designed to provide communication security over the Internet.", "Encryption"),
	LI_LIST_APP(LI_LDAP, ldap, IPPROTO_TCP, IPPROTO_MAX, "Lightweight Directory Access Protocol", "LDAP is a protocol for reading and editing directories over an IP network.", "Database"),
	LI_LIST_APP(LI_SNMP, snmp, IPPROTO_UDP, IPPROTO_MAX, "Simple Network Management Protocol", "SNMP is an Internet-standard protocol for managing devices on IP networks.", "Network Monitoring"),
	LI_LIST_APP(LI_BITTORRENT, bittorrent, IPPROTO_TCP, IPPROTO_MAX, "BitTorrent", "A peer-to-peer file sharing protocol used for transferring large amounts of data.", "File Transfer"),
	LI_LIST_APP(LI_GNUTELLA, gnutella, IPPROTO_TCP, IPPROTO_MAX, "Gnutella", "Gnutella is the protocol of the corresponding P2P network.", "File Transfer"),
	LI_LIST_APP(LI_IMPP, impp, IPPROTO_TCP, IPPROTO_MAX, "Instant Messaging and Presence Protocol", "IMPP is a protocol clients use to interact with an Instant Messaging server.", "Messaging"),
	LI_LIST_APP(LI_XMPP, xmpp, IPPROTO_TCP, IPPROTO_MAX, "Extensible Messaging and Presence Protocol", "XMPP is an open technology for real-time communication.", "Messaging"),
	LI_LIST_APP(LI_SYSLOG, syslog, IPPROTO_UDP, IPPROTO_TCP, "Syslog", "Syslog is a standard for forwarding log messages in an Internet Protocol (IP) computer network.", "Network Monitoring"),
	LI_LIST_APP(LI_L2TP, l2tp, IPPROTO_UDP, IPPROTO_MAX, "Layer 2 Tunneling Protocol", "L2TP is a tunneling protocol used to support virtual private networks (VPNs). It does not provide any encryption or confidentiality by itself; it relies on an encryption protocol that it passes within the tunnel to provide privacy.", "VPN and Tunneling"),
	LI_LIST_APP(LI_NTP, ntp, IPPROTO_UDP, IPPROTO_MAX, "Network Time Protocol", "NTP is used for synchronizing the clocks of computer systems over the network. Sends small packets with current date and time.", "Networking"),
	/* greedy protocols need to remain down here */
	LI_LIST_APP(LI_DNS, dns, IPPROTO_UDP, IPPROTO_TCP, "Domain Name System", "DNS is a protocol to resolve domain names to IP addresses.", "Networking"),
	LI_LIST_APP(LI_OPENVPN, openvpn, IPPROTO_UDP, IPPROTO_TCP, "OpenVPN", "OpenVPN is a free and open source virtual private network (VPN) program for creating point-to-point or server-to-multiclient encrypted tunnels between host computers. It is capable of establishing direct links between computers across network address translators (NATs) and firewalls.", "VPN and Tunneling"),
	LI_LIST_APP(LI_RTCP, rtcp, IPPROTO_UDP, IPPROTO_MAX, "Real-Time Transport Control Protocol", "RTCP is a sister protocol of the Real-time Transport Protocol (RTP). RTCP provides out-of-band control information for an RTP flow.", "Streaming Media"),
	LI_LIST_APP(LI_NETBIOS, netbios, IPPROTO_UDP, IPPROTO_TCP, "NetBIOS", "NetBIOS is an acronym for Network Basic Input/Output System. It provides services related to the session layer of the OSI model allowing applications on separate computers to communicate over a local area network.", "Networking"),
	LI_LIST_APP(LI_TELNET, telnet, IPPROTO_TCP, IPPROTO_MAX, "Telnet", "Telnet (teletype network) is a network protocol used on the Internet or local area networks to provide a bidirectional interactive text-oriented communications facility using a virtual terminal connection.", "Remote Access"),
	LI_LIST_APP(LI_RTP, rtp, IPPROTO_UDP, IPPROTO_MAX, "Real-Time Transport Protocol", "RTP is primarily used to deliver real-time audio and video.", "Streaming Media"),
};

unsigned int
peak_li_test(const struct peak_packet *packet, const unsigned int number)
{
	unsigned int i;

	/*
	 * Caution: All safeguards are off!
	 */

	for (i = 0; i < lengthof(apps); ++i) {
		if (apps[i].number == number &&
		    (apps[i].proto[0] == packet->net_type ||
		     apps[i].proto[1] == packet->net_type)) {
			return (apps[i].function(packet));
		}
	}

	return (0);
}

unsigned int
peak_li_get(const struct peak_packet *packet)
{
	unsigned int i;

	switch (packet->net_type) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
		/* payload needed for these */
		if (!packet->app_len) {
			return (LI_UNKNOWN);
		}
		/* FALLTHROUGH */
	default:
		break;
	}

	for (i = 0; i < lengthof(apps); ++i) {
		if ((apps[i].proto[0] == packet->net_type) ||
		    (apps[i].proto[1] == packet->net_type)) {
			if (apps[i].function(packet)) {
				return (apps[i].number);
			}
		}
	}

	/*
	 * Set `undefined' right away.  This makes it easier
	 * for a rules engine to do negation of policies.
	 * Only exception is a leakage from IP type detection:
	 * don't set it if we can't look...
	 */
	return (packet->app_len ? LI_UNDEFINED : LI_UNKNOWN);
}

unsigned int
peak_li_number(const char *name)
{
	unsigned int i;

	/*
	 * The reverse protocol lookup is mostly meant for use
	 * in a parser so that we can externalise the intel for
	 * all apps to the place where it comes from.  This is
	 * even better than rolling out script-generated lists.
	 */
	for (i = 0; i < lengthof(apps); ++i) {
		if (!strcasecmp(name, apps[i].name)) {
			return (apps[i].number);
		}
	}

	/*
	 * Don't forget to check against undefined protocols,
	 * which means no match was found in the known list.
	 */
	if (!strcasecmp(name, "undefined")) {
		return (LI_UNDEFINED);
	}

	/*
	 * LI_UNKNOWN cannot be blocked and is implicitly
	 * passed, so it's the perfect return value!
	 */
	return (LI_UNKNOWN);
}

static inline const char *
peak_li_data(const unsigned int number, const unsigned int offset)
{
	unsigned int i;

	switch (number) {
	case LI_UNKNOWN:
		return ("unknown");
	case LI_UNDEFINED:
		return ("undefined");
	default:
		for (i = 0; i < lengthof(apps); ++i) {
			if (number == apps[i].number) {
				register unsigned long *base = (unsigned long *)
				    (((char*) &apps[i]) + offset);

				return ((char *) *base);
			}
		}
		panic("application %u caused a failure\n", number);
		/* NOTREACHED */
	}
}

const char *
peak_li_name(const unsigned int number)
{
	return peak_li_data(number, __offsetof(struct peak_lis, name));
}

const char *
peak_li_pretty(const unsigned int number)
{
	return peak_li_data(number, __offsetof(struct peak_lis, pretty));
}

const char *
peak_li_desc(const unsigned int number)
{
	return peak_li_data(number, __offsetof(struct peak_lis, desc));
}

const char *
peak_li_cat(const unsigned int number)
{
	return peak_li_data(number, __offsetof(struct peak_lis, cat));
}
