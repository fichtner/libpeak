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

#ifndef PEAK_LOCATE_H
#define PEAK_LOCATE_H

#define LOCATE_MAGIC	0x10CA7E10CA7E7412ull
#define LOCATE_REVISION	0

struct peak_locate_hdr {
	uint64_t magic;
	uint32_t revision;
	uint32_t count;
};

struct peak_locate {
	struct netaddr min;
	struct netaddr max;
	char location[8];
};

static inline int
peak_locate_cmp(const void *xx, const void *yy)
{
	const struct peak_locate *x = xx;
	const struct peak_locate *y = yy;

	if (netcmp(&x->max, &y->min) < 0) {
		return (-1);
	} else if (netcmp(&x->min, &y->max) > 0) {
		return (1);
	}

	return (0);
}

struct peak_locates	*peak_locate_init(const char *);
void			 peak_locate_exit(struct peak_locates *);
const char		*peak_locate_me(struct peak_locates *,
			     const struct netaddr *);

#endif /* !PEAK_LOCATE_H */
