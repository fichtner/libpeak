#ifndef PEAK_TYPE_H
#define PEAK_TYPE_H

#include <stdint.h>

static inline uint16_t
bswap16(uint16_t u)
{
	return ((u << 8) | (u >> 8));
}

#define bswap32(x)	__builtin_bswap32(x)
#define bswap64(x)	__builtin_bswap64(x)

#ifdef __CHECKER__
#define __sync_nand_and_fetch(x, y)	(y)
#define __sync_fetch_and_nand(x, y)	(y)
#define __sync_and_and_fetch(x, y)	(y)
#define __sync_fetch_and_and(x, y)	(y)
#define __sync_add_and_fetch(x, y)	(y)
#define __sync_fetch_and_add(x, y)	(y)
#define __sync_sub_and_fetch(x, y)	(y)
#define __sync_fetch_and_sub(x, y)	(y)
#define __sync_xor_and_fetch(x, y)	(y)
#define __sync_fetch_and_xor(x, y)	(y)
#define __sync_or_and_fetch(x, y)	(y)
#define __sync_fetch_and_or(x, y)	(y)
#define __builtin_bswap32(x)		(x)
#define __builtin_bswap64(x)		(x)
#endif /* __CHECKER__ */

static inline uint16_t
le16dec(const void *pp)
{
	const unsigned char *p = pp;

	return (p[1] << 8 | p[0]);
}

static inline void
le16enc(void *pp, const uint16_t u)
{
	unsigned char *p = pp;

	p[0] = u;
	p[1] = u >> 8;
}

static inline uint32_t
le32dec(const void *pp)
{
	const unsigned char *p = pp;

	return ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

static inline void
le32enc(void *pp, const uint32_t u)
{
	unsigned char *p = pp;

	p[0] = u;
	p[1] = u >> 8;
	p[2] = u >> 16;
	p[3] = u >> 24;
}

static inline uint64_t
le64dec(const void *pp)
{
	const unsigned char *p = pp;

	return (((uint64_t)le32dec(p + 4) << 32) | le32dec(p));
}

static inline void
le64enc(void *pp, const uint64_t u)
{
	unsigned char *p = pp;

	le32enc(p, u);
	le32enc(p + 4, u >> 32);
}

static inline uint16_t
be16dec(const void *pp)
{
	const unsigned char *p = pp;

	return (p[1] | (p[0] << 8));
}

static inline void
be16enc(void *pp, const uint16_t u)
{
	unsigned char *p = pp;

	p[0] = u >> 8;
	p[1] = u;
}

static inline uint32_t
be32dec(const void *pp)
{
	const unsigned char *p = pp;

	return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static inline void
be32enc(void *pp, const uint32_t u)
{
	unsigned char *p = pp;

	p[0] = u >> 24;
	p[1] = u >> 16;
	p[2] = u >> 8;
	p[3] = u;
}

static inline uint64_t
be64dec(const void *pp)
{
	const unsigned char *p = pp;

	return (((uint64_t)be32dec(p) << 32) | be32dec(p + 4));
}

static inline void
be64enc(void *pp, const uint64_t u)
{
	unsigned char *p = pp;

	be32enc(p, u >> 32);
	be32enc(p + 4, u);
}

#define peak_uint16_wrap(__x__) ((uint16_t)(__x__) > UINT16_MAX / 2U)
#define peak_uint32_wrap(__x__) ((uint32_t)(__x__) > UINT32_MAX / 2U)
#define peak_uint64_wrap(__x__) ((uint64_t)(__x__) > UINT64_MAX / 2ULL)

#endif /* !PEAK_TYPE_H */
