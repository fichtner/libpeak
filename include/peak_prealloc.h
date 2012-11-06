#ifndef PEAK_PREALLOC_H
#define PEAK_PREALLOC_H

#define PEAK_PREALLOC_VALUE 0x1234ABBA5678AC97ull

struct peak_prealloc {
	SLIST_ENTRY(peak_prealloc) next;
	unsigned long long magic;
	unsigned char user[];
};

struct peak_preallocs {
	SLIST_HEAD(, peak_prealloc) free;
	size_t count, used, size, mem_size;
	void *mem;

	peak_spinlock_t lock;
};

#define PEAK_PREALLOC_TO_USER(__e__)	((void *)((__e__)->user))
#define PEAK_PREALLOC_FROM_USER(__e__)					\
    ((void *)(((unsigned char *)(__e__)) -				\
    sizeof(struct peak_prealloc)))

#define PEAK_PREALLOC_HEALTHY		0
#define PEAK_PREALLOC_UNDERFLOW		1
#define PEAK_PREALLOC_DOUBLE_FREE	2
#define PEAK_PREALLOC_MISSING_CHUNKS	3

#define PREALLOC_EMPTY(x)	SLIST_EMPTY(&(x)->free)

static inline void *
_peak_preget(struct peak_preallocs *self)
{
	struct peak_prealloc *e = SLIST_FIRST(&self->free);
	if (unlikely(!e)) {
		return (NULL);
	}

	SLIST_REMOVE_HEAD(&self->free, next);
	SLIST_NEXT(e, next) = NULL;
	++self->used;

	return (PEAK_PREALLOC_TO_USER(e));
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
__peak_preput(struct peak_preallocs *self, void *ptr)
{
	unsigned int ret = PEAK_PREALLOC_HEALTHY;

	if (unlikely(!ptr)) {
		return (ret);
	}

	struct peak_prealloc *e = PEAK_PREALLOC_FROM_USER(ptr);

	if (unlikely(PEAK_PREALLOC_VALUE != e->magic)) {
		return (PEAK_PREALLOC_UNDERFLOW);
	}

	if (unlikely(SLIST_NEXT(e, next) ||
	    e == SLIST_FIRST(&self->free))) {
		return (PEAK_PREALLOC_DOUBLE_FREE);
	}

	SLIST_INSERT_HEAD(&self->free, e, next);
	--self->used;

	return (ret);
}

#define _peak_preput(x, y) do {						\
	switch (__peak_preput(x, y)) {					\
	case PEAK_PREALLOC_HEALTHY:					\
		break;							\
	case PEAK_PREALLOC_UNDERFLOW:					\
		peak_panic("buffer underflow detected\n");		\
		/* NOTREACHED */					\
	case PEAK_PREALLOC_DOUBLE_FREE:					\
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
	if (!count || !size) {
		return (0);
	}

	if (size & 0x7) {
		/* only allow 8 byte aligned structs */
		return (0);
	}

	const size_t ptr_size = size + sizeof(struct peak_prealloc);
	const size_t mem_size = count * ptr_size;
	if (mem_size / count != ptr_size) {
		return (0);
	}

	bzero(self, sizeof(*self));

	peak_spin_init(&self->lock);
	self->mem_size = mem_size;
	self->count = count;
	self->size = size;
	self->used = 0;

	self->mem = peak_malign(mem_size);
	if (!self->mem) {
		return (0);
	}

	struct peak_prealloc *e = self->mem;
	struct peak_prealloc *f;
	size_t i;

	SLIST_INIT(&self->free);
	SLIST_INSERT_HEAD(&self->free, e, next);
	e->magic = PEAK_PREALLOC_VALUE;

	for (i = 1; i < count; ++i) {
		f = (void *)((unsigned char *) e + ptr_size);
		f->magic = PEAK_PREALLOC_VALUE;

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
	unsigned int ret = PEAK_PREALLOC_HEALTHY;

	if (!self) {
		return (ret);
	}

	if (self->used) {
		return (PEAK_PREALLOC_MISSING_CHUNKS);
	}

	peak_spin_exit(&self->lock);
	peak_free(self->mem);

	return (ret);
}

#define _peak_prefree(x) do {						\
	switch (__peak_prefree(x)) {					\
	case PEAK_PREALLOC_HEALTHY:					\
		break;							\
	case PEAK_PREALLOC_MISSING_CHUNKS:				\
		peak_panic("missing chunks detected\n");		\
		/* NOTREACHED */					\
	}								\
} while (0)

#define peak_prefree(x) do {						\
	_peak_prefree(x);						\
	peak_free(x);							\
} while (0)

#undef PEAK_PREALLOC_FROM_USER
#undef PEAK_PREALLOC_TO_USER
#undef PEAK_PREALLOC_VALUE

#endif /* !PEAK_PREALLOC_H */
