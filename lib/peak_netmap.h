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

extern const struct peak_transfers transfer_netmap;

#if defined(NETMAP_API) && NETMAP_API >= 11

unsigned int		 peak_netmap_divert(struct peak_transfer *,
			     const char *);
struct peak_transfer	*peak_netmap_claim(struct peak_transfer *,
			     int, const unsigned int);
unsigned int		 peak_netmap_forward(struct peak_transfer *);
unsigned int		 peak_netmap_drop(struct peak_transfer *);
unsigned int		 peak_netmap_attach(const char *);
unsigned int		 peak_netmap_detach(const char *);
void			 peak_netmap_unlock(void);
void			 peak_netmap_lock(void);

#endif

#endif /* __FreeBSD__ */

#endif /* !PEAK_NETMAP_H */
