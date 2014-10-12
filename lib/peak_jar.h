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

#ifndef PEAK_JAR_H
#define PEAK_JAR_H

struct peak_jars {
	TAILQ_HEAD(peak_jar_header, peak_jar_head) heads;
	unsigned int first_serial;
	unsigned int last_serial;
	unsigned char *buffer;
	size_t write;
	size_t read;
	size_t size;
};

struct peak_jar {
	TAILQ_HEAD(peak_jar_user, peak_jar_data) datas;
	unsigned int first_serial;
	unsigned int last_serial;
};

#define JAR_INIT(context) do {						\
	TAILQ_INIT(&(context)->datas);					\
} while  (0)

typedef unsigned int (*peak_jar_fun) (void *, void *, size_t len);

#define JAR_RETURN	0	/* keep, but return to caller */
#define JAR_DROP	1	/* drop the current user data */
#define JAR_KEEP	2	/* keep the current user data */

void		peak_jar_pack(struct peak_jars *, struct peak_jar *,
		    const void *, const size_t);
unsigned int	peak_jar_fifo(struct peak_jars *, struct peak_jar *,
		    peak_jar_fun, void *);
unsigned int	peak_jar_lifo(struct peak_jars *, struct peak_jar *,
		    peak_jar_fun, void *);
unsigned int	peak_jar_init(struct peak_jars *, size_t);
void		peak_jar_exit(struct peak_jars *);

#endif /* !PEAK_JAR_H */
