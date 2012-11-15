#ifndef PEAK_PREALLOC_H
#define PEAK_PREALLOC_H

#define PREALLOC_VALUE 0x1234ABBA5678AC97ull

struct peak_prealloc {
	SLIST_ENTRY(peak_prealloc) next;
	unsigned long long magic;
	unsigned char user[];
};

struct peak_preallocs {
	SLIST_HEAD(, peak_prealloc) free;
	size_t used;
	size_t size;
	void *mem_start;
	void *mem_stop;

	peak_spinlock_t lock;
};

#define PREALLOC_FROM_USER(x)	(((struct peak_prealloc *)(x)) - 1)
#define PREALLOC_TO_USER(x)	&(x)->user

#define PREALLOC_HEALTHY	0
#define PREALLOC_UNDERFLOW	1
#define PREALLOC_DOUBLE_FREE	2
#define PREALLOC_MISSING_CHUNKS	3

#define PREALLOC_VALID(x, y)	(((void *)(y) >= (x)->mem_start) &&	\
    ((void*)(y) < (x)->mem_stop))
#define PREALLOC_EMPTY(x)	SLIST_EMPTY(&(x)->free)
#define PREALLOC_USED(x)	(x)->used
#define PREALLOC_SIZE(x)	(x)->size

static inline void *
_peak_preget(struct peak_preallocs *self)
{
	struct peak_prealloc *e = SLIST_FIRST(&self->free);
	if (unlikely(!e)) {
		return (NULL);
	}

	SLIST_REMOVE_HEAD(&self->free, next);
	SLIST_NEXT(e, next) = NULL;
	++PREALLOC_USED(self);

	return (PREALLOC_TO_USER(e));
}

static inline void *
peak_preget(struct peak_preallocs *self)
{
	void *ret;

	peak_spin_lock(&self->lock);
	ret = _peak_preget(self);
	peak_spin_unlock(&self->lock);

	return (ret);
}

static inline unsigned int
__peak_preput(struct peak_preallocs *self, void *p)
{
	unsigned int ret = PREALLOC_HEALTHY;

	if (unlikely(!p)) {
		return (ret);
	}

	struct peak_prealloc *e = PREALLOC_FROM_USER(p);

	if (unlikely(PREALLOC_VALUE != e->magic)) {
		return (PREALLOC_UNDERFLOW);
	}

	if (unlikely(SLIST_NEXT(e, next) ||
	    e == SLIST_FIRST(&self->free))) {
		return (PREALLOC_DOUBLE_FREE);
	}

	SLIST_INSERT_HEAD(&self->free, e, next);
	--PREALLOC_USED(self);

	return (ret);
}

#define _peak_preput(x, y) do {						\
	switch (__peak_preput(x, y)) {					\
	case PREALLOC_HEALTHY:						\
		break;							\
	case PREALLOC_UNDERFLOW:					\
		peak_panic("buffer underflow detected\n");		\
		/* NOTREACHED */					\
	case PREALLOC_DOUBLE_FREE:					\
		peak_panic("double free detected\n");			\
		/* NOTREACHED */					\
	}								\
} while (0)

#define peak_preput(x, y) do {						\
	peak_spin_lock(&(x)->lock);					\
	_peak_preput(x, y);						\
	peak_spin_unlock(&(x)->lock);					\
} while (0)

static inline unsigned int
_peak_prealloc(struct peak_preallocs *self, size_t count, size_t size)
{
	bzero(self, sizeof(*self));

	if (!count || !size) {
		return (0);
	}

	if (size & 0x7) {
		/* only allow 8 byte aligned structs */
		return (0);
	}

	const size_t elm_size = size + sizeof(struct peak_prealloc);
	const size_t mem_size = count * elm_size;
	if (mem_size / count != elm_size) {
		return (0);
	}

	peak_spin_init(&self->lock);
	PREALLOC_SIZE(self) = size;
	PREALLOC_USED(self) = 0;

	self->mem_start = peak_malign(mem_size);
	if (!self->mem_start) {
		return (0);
	}

	self->mem_stop = ((unsigned char *)self->mem_start) + mem_size;

	struct peak_prealloc *e = self->mem_start;
	struct peak_prealloc *f;
	size_t i;

	SLIST_INIT(&self->free);
	SLIST_INSERT_HEAD(&self->free, e, next);
	e->magic = PREALLOC_VALUE;

	for (i = 1; i < count; ++i) {
		f = (void *)((unsigned char *) e + elm_size);
		f->magic = PREALLOC_VALUE;

		SLIST_INSERT_AFTER(e, f, next);

		e = f;
	}

	SLIST_NEXT(e, next) = NULL;

	return (1);
}

static inline struct peak_preallocs *
peak_prealloc(size_t count, size_t size)
{
	struct peak_preallocs *self = peak_malloc(sizeof(*self));
	if (!self) {
		return (NULL);
	}

	if (!_peak_prealloc(self, count, size)) {
		peak_free(self);
		return (NULL);
	}

	return (self);
}

static inline unsigned int
__peak_prefree(struct peak_preallocs *self)
{
	unsigned int ret = PREALLOC_HEALTHY;

	if (!self) {
		return (ret);
	}

	if (PREALLOC_USED(self)) {
		return (PREALLOC_MISSING_CHUNKS);
	}

	peak_spin_exit(&self->lock);
	peak_free(self->mem_start);

	return (ret);
}

#define _peak_prefree(x) do {						\
	switch (__peak_prefree(x)) {					\
	case PREALLOC_HEALTHY:						\
		break;							\
	case PREALLOC_MISSING_CHUNKS:					\
		peak_panic("missing chunks detected\n");		\
		/* NOTREACHED */					\
	}								\
} while (0)

#define peak_prefree(x) do {						\
	_peak_prefree(x);						\
	peak_free(x);							\
} while (0)

#undef PREALLOC_FROM_USER
#undef PREALLOC_TO_USER
#undef PREALLOC_VALUE

#endif /* !PEAK_PREALLOC_H */
