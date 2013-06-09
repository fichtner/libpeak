/*
 * Copyright (c) 2012 Franco Fichtner <franco@lastsummer.de>
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

#ifndef PEAK_EXALLOC_H
#define PEAK_EXALLOC_H

typedef struct {
	prealloc_t min;
	prealloc_t avg;
	prealloc_t max;
} exalloc_t;

struct peak_exalloc {
	prealloc_t *mem;
	unsigned char user[];
};

#define EXALLOC_FROM_USER(x)	(((struct peak_exalloc *)(x)) - 1)
#define EXALLOC_TO_USER(x)	&(x)->user

static inline void *
_exalloc_get(prealloc_t *mem)
{
	struct peak_exalloc *e = prealloc_get(mem);
	if (unlikely(!e)) {
		return (NULL);
	}

	e->mem = mem;

	return (EXALLOC_TO_USER(e));
}

static inline void *
_exalloc_put(void *p)
{
	struct peak_exalloc *e = EXALLOC_FROM_USER(p);

	prealloc_put(e->mem, e);

	return (NULL);
}

static inline prealloc_t *
_exalloc_match(exalloc_t *self, const size_t size,
    const prealloc_t *current)
{
	prealloc_t *list[] = {
		&self->min,
		&self->avg,
		&self->max,
	};
	unsigned int i;

	for (i = 0; i < lengthof(list); ++i) {
		if (size <= prealloc_size(list[i]) -
		    sizeof(struct peak_exalloc)) {
			if (!prealloc_empty(list[i]) ||
			    current == list[i]) {
				return (list[i]);
			}
		}
	}

	return (NULL);
}

static inline void *
_exalloc_swap(exalloc_t *self, void *p, const size_t size,
    size_t *copy_size)
{
	struct peak_exalloc *e = EXALLOC_FROM_USER(p);
	prealloc_t *mem;

	*copy_size = 0;

	mem = _exalloc_match(self, size, e->mem);
	if (!mem) {
		/* no matching chunks available */
		return (NULL);
	}

	if (mem == e->mem) {
		/* current chunk is big enough */
		return (p);
	}

	*copy_size = MIN(prealloc_size(e->mem), prealloc_size(mem)) -
	    sizeof(struct peak_exalloc);

	return (_exalloc_get(mem));
}

static inline void *
exalloc_swap(exalloc_t *self, void *p, size_t size)
{
	size_t copy_size;
	prealloc_t *mem;
	void *ret;

	switch ((!!p << 1) + !!size) {
	case 0x3:
		ret = _exalloc_swap(self, p, size, &copy_size);
		if (ret && copy_size) {
			bcopy(p, ret, copy_size);
			_exalloc_put(p);
		}
		break;
	case 0x2:
		ret = _exalloc_put(p);
		break;
	case 0x1:
		mem = _exalloc_match(self, size, NULL);
		if (mem) {
			ret = _exalloc_get(mem);
			break;
		}
		/* FALLTHROUGH */
	default:
		ret = NULL;
		break;
	}

	return (ret);
}

#define exalloc_get(x, y)	exalloc_swap(x, NULL, y)
#define exalloc_put(x, y)	exalloc_swap(x, y, 0)

#define exalloc_exit(self) do {						\
	prealloc_exit(&(self)->min);					\
	prealloc_exit(&(self)->avg);					\
	prealloc_exit(&(self)->max);					\
} while (0)

#define exalloc_exitd(self) do {					\
	if (self) {							\
		exalloc_exit(self);					\
		free(self);						\
	}								\
} while (0)

static inline unsigned int
exalloc_init(exalloc_t *self, size_t count, size_t min_size,
    size_t avg_size, size_t max_size)
{
	size_t min_count, avg_count, max_count;
	unsigned int ret = 1;

	bzero(self, sizeof(*self));

	if (count < 3 || !min_size || (min_size >= avg_size) ||
	    (avg_size >= max_size)) {
		return (0);
	}

	avg_count = count / 2;
	min_count = count / 3;
	max_count = count - avg_count - min_count;

	ret &= prealloc_init(&self->min, min_count, min_size +
	    sizeof(struct peak_exalloc));
	ret &= prealloc_init(&self->avg, avg_count, avg_size +
	    sizeof(struct peak_exalloc));
	ret &= prealloc_init(&self->max, max_count, max_size +
	    sizeof(struct peak_exalloc));
	if (!ret) {
		return (0);
	}

	return (1);
}

static inline exalloc_t *
exalloc_initd(size_t count, size_t min_size, size_t avg_size,
    size_t max_size)
{
	exalloc_t *self = malloc(sizeof(*self));
	if (!self) {
		return (NULL);
	}

	if (!exalloc_init(self, count, min_size, avg_size, max_size)) {
		exalloc_exitd(self);
		return (NULL);
	}

	return (self);
}

#undef EXALLOC_FROM_USER
#undef EXALLOC_TO_USER

#endif /* PEAK_EXALLOC_H */
