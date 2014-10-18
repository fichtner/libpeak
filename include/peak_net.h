/*
 * Copyright (c) 2012-2014 Franco Fichtner <franco@packetwerk.com>
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

#ifndef PEAK_NET_H
#define PEAK_NET_H

#include <arpa/inet.h>
#include <sys/socket.h>

struct netaddr {
	union {
		uint8_t byte[16];
		uint16_t word[8];
		uint32_t dword[4];
		uint64_t qword[2];
	} u;
};

#define netto4(x)	((const void *)&(x)->u.dword[3])
#define netto6(x)	((const void *)&(x)->u.dword[0])

static const struct netaddr net_in64 = {
	/* IPv4-mapped IPv6 address via ::ffff:0:0/96 */
	.u = {
		.word = {
			0x0, 0x0, 0x0, 0x0, 0x0, 0xffff, 0x0, 0x0,
		},
	},
};

static inline void
netaddr4(struct netaddr *in4, const uint32_t ip)
{
	in4->u.qword[0] = net_in64.u.qword[0];
	in4->u.qword[1] = net_in64.u.qword[1];
	in4->u.dword[3] = ip;
}

static inline void
netaddr6(struct netaddr *in6, const void *ip)
{
	/* IPv6 is native, but don't tell anyone */
	memcpy(in6, ip, sizeof(*in6));
}

static inline int
netcmp(const struct netaddr *x, const struct netaddr *y)
{
	return (memcmp(x, y, sizeof(*x)));
}

static inline int
netcmp4(const struct netaddr *x)
{
	return (memcmp(x, &net_in64, sizeof(net_in64) -
	    sizeof(net_in64.u.dword[0])));
}

static inline int
netcmp6(const struct netaddr *x)
{
	return (!netcmp4(x));
}

#define NET_PREFIX4(x)	((x) ? (x) + 96 : 0)
#define NET_PREFIX6(x)	(x)
#define NET_PREFIX_MAX	128

static inline int
netprefix(const struct netaddr *x, const struct netaddr *y, unsigned int yp)
{
	const unsigned char prefix[] = {
		0xFF, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE,
	};
	unsigned int i;
	int ret;

	if (!yp) {
		return (1);
	}

	--yp;

	if (yp >= NET_PREFIX_MAX) {
		return (1);
	}

	for (i = 0; yp >= lengthof(prefix); ++i, yp -= lengthof(prefix)) {
		ret = x->u.byte[i] - y->u.byte[i];
		if (ret) {
			return (ret);
		}
	}

	return ((x->u.byte[i] & prefix[yp]) -
	    (y->u.byte[i] & prefix[yp]));
}

#define netprint(x) ({							\
	const int _nf = netprefix(x, &net_in64, 96) ? AF_INET6 : AF_INET;\
	(inet_ntop(_nf, ((_nf == AF_INET) ? netto4(x) : netto6(x)),	\
	    (char [INET6_ADDRSTRLEN]) {}, INET6_ADDRSTRLEN));		\
})

#define NET_PORTSTRLEN	8

#define portprint(y) ({							\
	char _port[NET_PORTSTRLEN] = "";				\
	if (likely((y) && !((y) & ~0xFFFF))) {				\
		snprintf(_port, sizeof(_port), ":%hu", (uint16_t)y);	\
	}								\
	(_port);							\
})

#endif /* !PEAK_NET_H */
