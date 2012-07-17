#ifndef PEAK_PREALLOC_H
#define PEAK_PREALLOC_H

#include <libkern/OSAtomic.h>

#define PEAK_PREALLOC_VALUE 0x1234ABBA5678AC97ULL

struct peak_prealloc_element {
	struct peak_prealloc_element *next;
	u64 magic;
	u8 user[];
};

struct peak_prealloc_struct {
	struct peak_prealloc_element *free;
	size_t count, used, size, mem_size;
	void *mem;

	OSSpinLock lock;
};

#define PEAK_PREALLOC_TO_USER(__e__)		((void *)((__e__)->user))
#define PEAK_PREALLOC_FROM_USER(__e__)									\
	((void *)(((u8 *)(__e__)) - sizeof(struct peak_prealloc_element)))

#define PEAK_PREALLOC_HEALTHY			0
#define PEAK_PREALLOC_UNDERFLOW			1
#define PEAK_PREALLOC_DOUBLE_FREE		2
#define PEAK_PREALLOC_MISSING_CHUNKS	3

static inline void *_peak_preget(struct peak_prealloc_struct *ptr)
{
	struct peak_prealloc_element *e = ptr->free;
	if (unlikely(!e)) {
		return NULL;
	}

	ptr->free = e->next;
	++ptr->used;

	e->next = NULL;

	return PEAK_PREALLOC_TO_USER(e);
}

static inline void *peak_preget(struct peak_prealloc_struct *ptr)
{
	void *ret;

	OSSpinLockLock(&ptr->lock);
	ret = _peak_preget(ptr);
	OSSpinLockUnlock(&ptr->lock);

	return ret;
}

static inline u32 __peak_preput(struct peak_prealloc_struct *ptr, void *chunk)
{
	u32 ret = PEAK_PREALLOC_HEALTHY;

	if (unlikely(!chunk)) {
		return ret;
	}

	struct peak_prealloc_element *e = PEAK_PREALLOC_FROM_USER(chunk);

	if (unlikely(PEAK_PREALLOC_VALUE != e->magic)) {
		return PEAK_PREALLOC_UNDERFLOW;
	}

	if (unlikely(e->next || e == ptr->free)) {
		return PEAK_PREALLOC_DOUBLE_FREE;
	}

	e->next = ptr->free;
	ptr->free = e;
	--ptr->used;

	return ret;
}

#define _peak_preput(__ptr__, __chunk__)					\
	do {													\
		switch(__peak_preput(__ptr__, __chunk__)) {			\
		case PEAK_PREALLOC_HEALTHY:							\
			break;											\
		case PEAK_PREALLOC_UNDERFLOW:						\
			peak_panic("buffer underflow detected\n");		\
		case PEAK_PREALLOC_DOUBLE_FREE:						\
			peak_panic("double free detected\n");			\
		}													\
	} while (0)

#define peak_preput(__ptr__, __chunk__)		\
	do {									\
		OSSpinLockLock(&(__ptr__)->lock);	\
		_peak_preput(__ptr__, __chunk__);	\
		OSSpinLockUnlock(&(__ptr__)->lock);	\
	} while (0)

static inline struct peak_prealloc_struct *peak_prealloc(size_t count, size_t size)
{
	if (!count || !size) {
		return NULL;
	}

	if (size & 0x7) {
		/* only allow 8 byte aligned structs */
		return NULL;
	}

	const size_t chunk_size = size + sizeof(struct peak_prealloc_element);
	const size_t mem_size = count * chunk_size;
	if (mem_size / count != chunk_size) {
		return NULL;
	}

	struct peak_prealloc_struct *ptr = peak_zalloc(sizeof(*ptr));
	if (!ptr) {
		return NULL;
	}

	ptr->mem_size = mem_size;
	ptr->count = count;
	ptr->size = size;
	ptr->used = 0;

	ptr->mem = peak_malign(mem_size);
	if (!ptr->mem) {
		peak_free(ptr);
		return NULL;
	}

	struct peak_prealloc_element *e = ptr->free = ptr->mem;

	for (;;) {
		e->next = (void *)((u8 *) e + chunk_size);
		e->magic = PEAK_PREALLOC_VALUE;

		if (!--count) {
			break;
		}

		e = e->next;
	}

	e->next = NULL;

	return ptr;
}

static inline u32 _peak_prefree(struct peak_prealloc_struct *ptr)
{
	u32 ret = PEAK_PREALLOC_HEALTHY;

	if (!ptr) {
		return ret;
	}

	if (ptr->used) {
		return PEAK_PREALLOC_MISSING_CHUNKS;
	}

	peak_free(ptr->mem);
	peak_free(ptr);

	return ret;
}

#define peak_prefree(__ptr__)							\
	do {												\
		switch (_peak_prefree(__ptr__)) {				\
		case PEAK_PREALLOC_HEALTHY:						\
			break;										\
		case PEAK_PREALLOC_MISSING_CHUNKS:				\
			peak_panic("missing chunks detected\n");	\
		}												\
	} while (0)

#undef PEAK_PREALLOC_FROM_USER
#undef PEAK_PREALLOC_TO_USER
#undef PEAK_PREALLOC_VALUE

#endif /* PEAK_PREALLOC_H */
