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

#ifndef PEAK_TRANSFER_H
#define PEAK_TRANSFER_H

struct peak_transfer {
	struct peak_timeval ts;
	const char *ifname;
	void *private_data;
	unsigned int len;
	unsigned int ll;
	void *buf;
};

struct peak_transfers {
	unsigned int (*divert)(struct peak_transfer *, const char *);
	struct peak_transfer *(*claim)(struct peak_transfer *, int,
	    const unsigned int);
	unsigned int (*forward)(struct peak_transfer *);
	unsigned int (*drop)(struct peak_transfer *);
	unsigned int (*attach)(const char *);
	unsigned int (*detach)(const char *);
	void (*unlock)(void);
	void (*lock)(void);
};

static inline unsigned int
peak_transfer_divert(struct peak_transfer *x, const char *y)
{
	(void)x;
	(void)y;

	return (1);
}

static inline struct peak_transfer *
peak_transfer_claim(struct peak_transfer *x, int y, const unsigned int z)
{
	(void)x;
	(void)y;
	(void)z;

	return (NULL);
}

static inline unsigned int
peak_transfer_forward(struct peak_transfer *x)
{
	(void)x;

	return (1);
}

static inline unsigned int
peak_transfer_drop(struct peak_transfer *x)
{
	(void)x;

	return (1);
}

static inline unsigned int
peak_transfer_attach(const char *x)
{
	(void)x;

	return (1);
}

static inline unsigned int
peak_transfer_detach(const char *x)
{
	(void)x;

	return (1);
}

static inline void
peak_transfer_unlock(void)
{
}

static inline void
peak_transfer_lock(void)
{
}

#endif /* PEAK_TRANSFER_H */
