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

#ifndef PEAK_PREALLOC_H
#define PEAK_PREALLOC_H

struct peak_prealloc {
	SLIST_ENTRY(peak_prealloc) next;
};

typedef struct {
	SLIST_HEAD(, peak_prealloc) free;
	size_t used;
	size_t size;
	void *mem_start;
	void *mem_stop;
	spinlock_t lock;
} prealloc_t;

#define prealloc_valid(self, p)	(((void *)(p) >= (self)->mem_start) &&	\
    ((void*)(p) < (self)->mem_stop))
#define prealloc_empty(self)	SLIST_EMPTY(&(self)->free)
#define prealloc_used(self)	(self)->used
#define prealloc_size(self)	(self)->size

static inline void *
prealloc_get(prealloc_t *self)
{
	struct peak_prealloc *e = SLIST_FIRST(&self->free);
	if (unlikely(!e)) {
		return (NULL);
	}

	SLIST_REMOVE_HEAD(&self->free, next);
	++prealloc_used(self);

	return (e);
}

static inline void *
prealloc_gets(prealloc_t *self)
{
	void *ret;

	spin_lock(&self->lock);
	ret = prealloc_get(self);
	spin_unlock(&self->lock);

	return (ret);
}

static inline void
prealloc_put(prealloc_t *self, void *p)
{
	struct peak_prealloc *e = p;
	if (likely(e)) {
		SLIST_INSERT_HEAD(&self->free, e, next);
		--prealloc_used(self);
	}
}

static inline void
prealloc_puts(prealloc_t *self, void *p)
{
	spin_lock(&self->lock);
	prealloc_put(self, p);
	spin_unlock(&(self)->lock);
}

static inline unsigned int
prealloc_init(prealloc_t *self, size_t count, size_t size)
{
	bzero(self, sizeof(*self));

	if (!count || !size) {
		return (0);
	}

	if (size & 0x7) {
		/* only allow 8 byte aligned structs */
		return (0);
	}

	self->mem_start = malign(count, size);
	if (!self->mem_start) {
		return (0);
	}

	prealloc_size(self) = size;
	prealloc_used(self) = 0;
	spin_init(&self->lock);

	self->mem_stop = ((unsigned char *)self->mem_start) +
	    (count * size);

	struct peak_prealloc *e = self->mem_start;
	struct peak_prealloc *f;
	size_t i;

	SLIST_INIT(&self->free);
	SLIST_INSERT_HEAD(&self->free, e, next);

	for (i = 1; i < count; ++i) {
		f = (void *)((unsigned char *)e + size);
		SLIST_INSERT_AFTER(e, f, next);
		e = f;
	}

	return (1);
}

static inline prealloc_t *
prealloc_initd(size_t count, size_t size)
{
	prealloc_t *self = malloc(sizeof(*self));
	if (!self) {
		return (NULL);
	}

	if (!prealloc_init(self, count, size)) {
		free(self);
		return (NULL);
	}

	return (self);
}

static inline unsigned int
_prealloc_exit(prealloc_t *self)
{
	if (!self) {
		return (0);
	}

	if (prealloc_used(self)) {
		return (1);
	}

	spin_exit(&self->lock);
	free(self->mem_start);

	return (0);
}

#define prealloc_exit(self) do {					\
	if (_prealloc_exit(self)) {					\
		panic("missing chunks detected\n");			\
		/* NOTREACHED */					\
	}								\
} while (0)

#define prealloc_exitd(self) do {					\
	prealloc_exit(self);						\
	free(self);							\
} while (0)

#endif /* !PEAK_PREALLOC_H */
