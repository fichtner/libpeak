/*
 * Copyright (c) 2014 Franco Fichtner <franco@packetwerk.com>
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

#ifndef PEAK_AUDIT_H
#define PEAK_AUDIT_H

enum {
	AUDIT_PACKET_DROP_LINK,
	AUDIT_PACKET_DROP_IPV4,
	AUDIT_PACKET_DROP_IPV6,
	AUDIT_PACKET_DROP_ICMP,
	AUDIT_PACKET_DROP_TCP,
	AUDIT_PACKET_DROP_UDP,
	AUDIT_TRACK_ADDED,
	AUDIT_TRACK_RECYCLED,
	AUDIT_TRACK_FAILED,
	AUDIT_MAX	/* last element */
};

struct peak_audit {
	uint64_t field[AUDIT_MAX];
};

#ifndef WITHOUT_AUDIT

void		 peak_audit_set(const unsigned int, const uint64_t);
void		 peak_audit_add(const unsigned int, const uint64_t);
void		 peak_audit_sync(struct peak_audit *);
const char	*peak_audit_name(const unsigned int);
uint64_t	 peak_audit_get(const unsigned int);
void		 peak_audit_inc(const unsigned int);

#else /* WITHOUT_AUDIT */

#define peak_audit_set(x, y)
#define peak_audit_add(x, y)
#define peak_audit_sync(x) do { (void)(x); } while (0);
#define peak_audit_name(x)	NULL
#define peak_audit_get(x)	0
#define peak_audit_inc(x)

#endif /* !WITHOUT_AUDIT */

#endif /* PEAK_AUDIT_H */
