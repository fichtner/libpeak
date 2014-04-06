/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
 * Copyright (c) 2014 Masoud Chelongar <masoud@packetwerk.com>
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

#ifndef PEAK_STRING_H
#define PEAK_STRING_H

enum {
	STRING_LOOSE,
	STRING_LEFT,
	STRING_RIGHT,
	STRING_EXACT,
	STRING_MAX	/* last element */
};

struct peak_strings	*peak_string_init(void);
void			 peak_string_find(struct peak_strings *,
			     const char *, const size_t, stash_t);
int			 peak_string_add(struct peak_strings *,
			     const unsigned int, const char *,
			     const size_t, const unsigned int);
void			 peak_string_exit(struct peak_strings *);

#endif /* !PEAK_STRING_H */
