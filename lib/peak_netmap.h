/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
 * Copyright (c) 2014 Alexey Saushev <alexey@packetwerk.com>
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

#ifndef PEAK_NETMAP_H
#define PEAK_NETMAP_H

#ifdef __FreeBSD__
#include <net/if.h>
#include <net/netmap.h>
#endif /* __FreeBSD__ */

struct peak_netmap {
	struct peak_timeval ts;
	const char *ifname;
	unsigned int len;
	unsigned int ll;
	void *buf;
};

#if defined(__FreeBSD__) && defined(NETMAP_API) && (NETMAP_API >= 11)

unsigned int		 peak_netmap_divert(struct peak_netmap *,
			     const char *);
struct peak_netmap	*peak_netmap_claim(int, const unsigned int);
unsigned int		 peak_netmap_forward(struct peak_netmap *);
unsigned int		 peak_netmap_drop(struct peak_netmap *);
unsigned int		 peak_netmap_attach(const char *);
unsigned int		 peak_netmap_detach(const char *);
void			 peak_netmap_unlock(void);
void			 peak_netmap_lock(void);

#else /* !__FreeBSD__ */

#define peak_netmap_divert(x, y)	1
#define peak_netmap_claim(x, y)		NULL
#define peak_netmap_forward(x)		do { (void)(x); } while (0)
#define peak_netmap_drop(x)		do { (void)(x); } while (0)
#define peak_netmap_attach(x)		1
#define peak_netmap_detach(x)		do { (void)(x); } while (0)
#define peak_netmap_unlock()		do { } while (0)
#define peak_netmap_lock()		do { } while (0)

#endif /* __FreeBSD__ */

#endif /* !PEAK_NETMAP_H */
