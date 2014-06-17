/*-
 * Copyright (c) 2002 Thomas Moestl <tmm@FreeBSD.org>
 * Copyright (c) 2012-2014 Franco Fichtner <franco@packetwerk.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef PEAK_TYPE_H
#define PEAK_TYPE_H

#include <stdint.h>

static inline uint16_t
peak_bswap16(uint16_t u)
{
	return ((u << 8) | (u >> 8));
}

#ifdef __builtin_bswap32
#define peak_bswap32(x)	__builtin_bswap32(x)
#else /* !__builtin_bswap32 */
static inline uint32_t
peak_bswap32(uint32_t u)
{
	return (((uint32_t)peak_bswap16(u)) << 16) | peak_bswap16(u >> 16);
}
#endif /* __builtin_bswap32 */

#ifdef __builtin_bswap64
#define peak_bswap64(x)	__builtin_bswap64(x)
#else /* !__builtin_bswap64 */
static inline uint64_t
peak_bswap64(uint64_t u)
{
	return (((uint64_t)peak_bswap32(u)) << 32) | peak_bswap32(u >> 32);
}
#endif /* __builtin_bswap64 */

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
peak_le16dec(const void *pp)
{
	const unsigned char *p = pp;

	return (p[1] << 8 | p[0]);
}

static inline void
peak_le16enc(void *pp, const uint16_t u)
{
	unsigned char *p = pp;

	p[0] = u;
	p[1] = u >> 8;
}

static inline uint32_t
peak_le32dec(const void *pp)
{
	const unsigned char *p = pp;

	return ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

static inline void
peak_le32enc(void *pp, const uint32_t u)
{
	unsigned char *p = pp;

	p[0] = u;
	p[1] = u >> 8;
	p[2] = u >> 16;
	p[3] = u >> 24;
}

static inline uint64_t
peak_le64dec(const void *pp)
{
	const unsigned char *p = pp;

	return (((uint64_t)peak_le32dec(p + 4) << 32) | peak_le32dec(p));
}

static inline void
peak_le64enc(void *pp, const uint64_t u)
{
	unsigned char *p = pp;

	peak_le32enc(p, u);
	peak_le32enc(p + 4, u >> 32);
}

static inline uint16_t
peak_be16dec(const void *pp)
{
	const unsigned char *p = pp;

	return (p[1] | (p[0] << 8));
}

static inline void
peak_be16enc(void *pp, const uint16_t u)
{
	unsigned char *p = pp;

	p[0] = u >> 8;
	p[1] = u;
}

static inline uint32_t
peak_be32dec(const void *pp)
{
	const unsigned char *p = pp;

	return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static inline void
peak_be32enc(void *pp, const uint32_t u)
{
	unsigned char *p = pp;

	p[0] = u >> 24;
	p[1] = u >> 16;
	p[2] = u >> 8;
	p[3] = u;
}

static inline uint64_t
peak_be64dec(const void *pp)
{
	const unsigned char *p = pp;

	return (((uint64_t)peak_be32dec(p) << 32) | peak_be32dec(p + 4));
}

static inline void
peak_be64enc(void *pp, const uint64_t u)
{
	unsigned char *p = pp;

	peak_be32enc(p, u >> 32);
	peak_be32enc(p + 4, u);
}

#define wrap16(x)	((uint16_t)(x) > UINT16_MAX / 2U)
#define wrap32(x)	((uint32_t)(x) > UINT32_MAX / 2U)
#define wrap64(x)	((uint64_t)(x) > UINT64_MAX / 2ULL)

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/endian.h>
#else /* !__NetBSD__ && !__FreeBSD__ */
#define bswap16	peak_bswap16
#define bswap32	peak_bswap32
#define bswap64	peak_bswap64
#define le16dec	peak_le16dec
#define le16enc	peak_le16enc
#define le32dec	peak_le32dec
#define le32enc	peak_le32enc
#define le64dec	peak_le64dec
#define le64enc	peak_le64enc
#define be16dec	peak_be16dec
#define be16enc	peak_be16enc
#define be32dec	peak_be32dec
#define be32enc	peak_be32enc
#define be64dec	peak_be64dec
#define be64enc	peak_be64enc
#endif /* __NetBSD__ || __FreeBSD__ */

#endif /* !PEAK_TYPE_H */
