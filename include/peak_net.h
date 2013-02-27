#ifndef PEAK_NET_H
#define PEAK_NET_H

struct netmap {
	union {
		uint8_t byte[16];
		uint16_t word[8];
		uint32_t dword[4];
		uint64_t qword[2];
	} u;
};

static inline void
netmap4(struct netmap *in4, const uint32_t ip)
{
	static const struct netmap _in6 = {
		/* IPv4-mapped IPv6 address via ::ffff:0:0/96 */
		.u.word = {
			0x0, 0x0, 0x0, 0x0, 0x0, 0xffff, 0x0, 0x0,
		},
	};

	in4->u.qword[0] = _in6.u.qword[0];
	in4->u.qword[1] = _in6.u.qword[1];
	in4->u.dword[3] = ip;
}

static inline void
netmap6(struct netmap *in6, const void *ip)
{
	/* IPv6 is native, but don't tell anyone */
	bcopy(ip, in6, sizeof(*in6));
}

static inline int
netcmp(const struct netmap *x, const struct netmap *y)
{
	return (memcmp(x, y, sizeof(*x)));
}

#endif /* !PEAK_NET_H */
