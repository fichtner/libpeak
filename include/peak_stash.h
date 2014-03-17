/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
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

#ifndef PEAK_STASH_H
#define PEAK_STASH_H

#define STASH_DECLARE(name, type, count)				\
struct peak_stash_##name {						\
	unsigned int index;						\
	unsigned int size;						\
	type values[count];						\
} _##name = {								\
	.size = count,							\
	.index = 0,							\
};									\
struct peak_stash_##name *name = &_##name;

#define STASH_REBUILD(name, type, ptr)					\
struct peak_stash_##name {						\
	unsigned int index;						\
	unsigned int size;						\
	type values[];							\
} *name = ptr;

#define STASH_FULL(name)	((name)->index >= (name)->size)
#define STASH_EMPTY(name)	!(name)->index
#define STASH_COUNT(name)	(name)->index
#define STASH_LIST(name)	(name)->values

#define STASH_CLEAR(name) do {						\
	(name)->index = 0;						\
} while (0)

#define STASH_PUSH(x, name) ({						\
	typeof(&(name)->values[0]) _ret = NULL;				\
	if (likely(!STASH_FULL(name))) {				\
		_ret = &(name)->values[(name)->index++];		\
		*_ret = *(x);						\
	}								\
	_ret;								\
})

#define STASH_POP(name) ({	 					\
	typeof(&(name)->values[0]) _ret = NULL;				\
	if (likely(!STASH_EMPTY(name))) {				\
		_ret = &(name)->values[--(name)->index];		\
	}								\
	_ret;								\
})

#define STASH_FOREACH(x, name)						\
	if (likely(!STASH_EMPTY(name)))					\
		for ((x) = &(name)->values[0];				\
		     (x) < &(name)->values[STASH_COUNT(name)];		\
		     (x)++)

#define STASH_FOREACH_REVERSE(x, name)					\
	if (likely(!STASH_EMPTY(name)))					\
		for ((x) = &(name)->values[STASH_COUNT(name) - 1];	\
		     (x) >= &(name)->values[0];				\
		     (x)--)

#define STASH_SORT(name, cmp) do {					\
	qsort(STASH_LIST(name), STASH_COUNT(name),			\
	    sizeof(*STASH_LIST(name)), cmp);				\
} while (0)

typedef void * stash_t;

#endif /* !PEAK_STASH_H */
