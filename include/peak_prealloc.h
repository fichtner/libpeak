#ifndef PEAK_PREALLOC_H
#define PEAK_PREALLOC_H

#define PEAK_PREALLOC_VALUE 0x1234ABBA5678AC97ull

struct _peak_prealloc {
	SLIST_ENTRY(_peak_prealloc) next;
	unsigned long long magic;
	unsigned char user[];
};

struct peak_prealloc {
	SLIST_HEAD(, _peak_prealloc) free;
	size_t count, used, size, mem_size;
	void *mem;

	peak_spinlock_t lock;
};

#define PEAK_PREALLOC_TO_USER(__e__)	((void *)((__e__)->user))
#define PEAK_PREALLOC_FROM_USER(__e__)					\
    ((void *)(((unsigned char *)(__e__)) -				\
    sizeof(struct _peak_prealloc)))

#define PEAK_PREALLOC_HEALTHY		0
#define PEAK_PREALLOC_UNDERFLOW		1
#define PEAK_PREALLOC_DOUBLE_FREE	2
#define PEAK_PREALLOC_MISSING_CHUNKS	3

#define PREALLOC_EMPTY(x)	SLIST_EMPTY(&(x)->free)

static inline void *
_peak_preget(struct peak_prealloc *ptr)
{
	struct _peak_prealloc *e = SLIST_FIRST(&ptr->free);
	if (unlikely(!e)) {
		return (NULL);
	}

	SLIST_REMOVE_HEAD(&ptr->free, next);
	SLIST_NEXT(e, next) = NULL;
	++ptr->used;

	return (PEAK_PREALLOC_TO_USER(e));
}

static inline void *
peak_preget(struct peak_prealloc *ptr)
{
	void *ret;

	peak_spin_lock(&ptr->lock);
	ret = _peak_preget(ptr);
	peak_spin_unlock(&ptr->lock);

	return (ret);
}

static inline unsigned int
__peak_preput(struct peak_prealloc *ptr, void *chunk)
{
	unsigned int ret = PEAK_PREALLOC_HEALTHY;

	if (unlikely(!chunk)) {
		return (ret);
	}

	struct _peak_prealloc *e = PEAK_PREALLOC_FROM_USER(chunk);

	if (unlikely(PEAK_PREALLOC_VALUE != e->magic)) {
		return (PEAK_PREALLOC_UNDERFLOW);
	}

	if (unlikely(SLIST_NEXT(e, next) ||
	    e == SLIST_FIRST(&ptr->free))) {
		return (PEAK_PREALLOC_DOUBLE_FREE);
	}

	SLIST_INSERT_HEAD(&ptr->free, e, next);
	--ptr->used;

	return (ret);
}

#define _peak_preput(__ptr__, __chunk__) do {				\
	switch (__peak_preput(__ptr__, __chunk__)) {			\
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

#define peak_preput(__ptr__, __chunk__) do {				\
	peak_spin_lock(&(__ptr__)->lock);				\
	_peak_preput(__ptr__, __chunk__);				\
	peak_spin_unlock(&(__ptr__)->lock);				\
} while (0)

static inline struct peak_prealloc *
peak_prealloc(size_t count, size_t size)
{
	if (!count || !size) {
		return (NULL);
	}

	if (size & 0x7) {
		/* only allow 8 byte aligned structs */
		return (NULL);
	}

	const size_t chunk_size = size + sizeof(struct _peak_prealloc);
	const size_t mem_size = count * chunk_size;
	if (mem_size / count != chunk_size) {
		return (NULL);
	}

	struct peak_prealloc *ptr = peak_zalloc(sizeof(*ptr));
	if (!ptr) {
		return (NULL);
	}

	peak_spin_init(&ptr->lock);
	ptr->mem_size = mem_size;
	ptr->count = count;
	ptr->size = size;
	ptr->used = 0;

	ptr->mem = peak_malign(mem_size);
	if (!ptr->mem) {
		peak_free(ptr);
		return (NULL);
	}

	struct _peak_prealloc *e = ptr->mem;
	struct _peak_prealloc *f;
	size_t i;

	SLIST_INIT(&ptr->free);
	SLIST_INSERT_HEAD(&ptr->free, e, next);
	e->magic = PEAK_PREALLOC_VALUE;

	for (i = 1; i < count; ++i) {
		f = (void *)((unsigned char *) e + chunk_size);
		f->magic = PEAK_PREALLOC_VALUE;

		SLIST_INSERT_AFTER(e, f, next);

		e = f;
	}

	SLIST_NEXT(e, next) = NULL;

	return (ptr);
}

static inline unsigned int
_peak_prefree(struct peak_prealloc *ptr)
{
	unsigned int ret = PEAK_PREALLOC_HEALTHY;

	if (!ptr) {
		return (ret);
	}

	if (ptr->used) {
		return (PEAK_PREALLOC_MISSING_CHUNKS);
	}

	peak_spin_exit(&ptr->lock);
	peak_free(ptr->mem);
	peak_free(ptr);

	return (ret);
}

#define peak_prefree(__ptr__) do {					\
	switch (_peak_prefree(__ptr__)) {				\
	case PEAK_PREALLOC_HEALTHY:					\
		break;							\
	case PEAK_PREALLOC_MISSING_CHUNKS:				\
		peak_panic("missing chunks detected\n");		\
		/* NOTREACHED */					\
	}								\
} while (0)

#undef PEAK_PREALLOC_FROM_USER
#undef PEAK_PREALLOC_TO_USER
#undef PEAK_PREALLOC_VALUE

#endif /* !PEAK_PREALLOC_H */
