/*
 * Copyright (c) 2013 Franco Fichtner <franco@packetwerk.com>
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

struct peak_netmap {
	const char *ifname;
	unsigned int len;
	unsigned int ll;
	uint64_t ts_ms;
	time_t ts_unix;
	void *buf;
};

unsigned int		 peak_netmap_forward(struct peak_netmap *,
			     const char *);
unsigned int		 peak_netmap_attach(const char *);
unsigned int		 peak_netmap_detach(const char *);
void			 peak_netmap_unlock(void);
void			 peak_netmap_lock(void);
struct peak_netmap	*peak_netmap_claim(int);

#endif /* !PEAK_NETMAP_H */
