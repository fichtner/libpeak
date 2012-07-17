#ifndef PEAK_ALLOC_H
#define PEAK_ALLOC_H

#include <stdlib.h>
#include <string.h>

#define PEAK_CACHELINE_SIZE 64

static inline size_t peak_cacheline_aligned(const size_t size)
{
	/* this is a pure masterpiece ;) */
	return size + PEAK_CACHELINE_SIZE - (size % PEAK_CACHELINE_SIZE ? : PEAK_CACHELINE_SIZE);
}

#define PEAK_ALLOC_HEALTHY		0
#define PEAK_ALLOC_UNDERFLOW	1
#define PEAK_ALLOC_OVERFLOW		2

#define peak_alloc_error(__code__)						\
	do {												\
		switch (__code__) {								\
		case PEAK_ALLOC_HEALTHY:						\
			break;										\
		case PEAK_ALLOC_UNDERFLOW:						\
			peak_panic("buffer underflow detected\n");	\
		case PEAK_ALLOC_OVERFLOW:						\
			peak_panic("buffer overflow detected\n");	\
		}												\
	} while (0)

#define PEAK_MEMALIGN_VALUE	0x8897A6B5C4D3E2F1ULL
#define PEAK_MALLOC_VALUE	0x1F2E3D4C5B6A7988ULL

#define PEAK_MALLOC_MEMALIGN_START	((u8 *) ptr - sizeof(u64))

#define PEAK_MALLOC_STOP_POS	((u8 *) ptr + 2 * sizeof(u64) + size)
#define PEAK_MALLOC_TO_USER		((u8 *) ptr + 2 * sizeof(u64))
#define PEAK_MALLOC_FROM_USER	((u8 *) ptr - 2 * sizeof(u64))
#define PEAK_MALLOC_START_POS	((u8 *) ptr + sizeof(u64))
#define PEAK_MALLOC_PADDING		(3 * sizeof(u64))
#define PEAK_MALLOC_SIZE_POS	ptr

static inline void *_peak_finalize_malloc(const u64 size, void *ptr)
{
	if (unlikely(!ptr)) {
		return NULL;
	}

	peak_put_u64(size, PEAK_MALLOC_SIZE_POS);
	peak_put_u64(PEAK_MALLOC_VALUE, PEAK_MALLOC_START_POS);
	peak_put_u64(PEAK_MALLOC_VALUE, PEAK_MALLOC_STOP_POS);

	return PEAK_MALLOC_TO_USER;
}

static inline u32 _peak_check_malloc(void *ptr)
{
	u32 ret = PEAK_ALLOC_HEALTHY;
	u64 size;

	if (unlikely(!ptr)) {
		return ret;
	}

	/* tricky macros, don't reorder */
	ptr = PEAK_MALLOC_FROM_USER;
	size = peak_get_u64(PEAK_MALLOC_SIZE_POS);

	if (unlikely(PEAK_MALLOC_VALUE != peak_get_u64(PEAK_MALLOC_STOP_POS))) {
		ret = PEAK_ALLOC_OVERFLOW;
	}

	return ret;
}

static inline void *peak_malloc(size_t size)
{
	if (unlikely(!size)) {
		return NULL;
	}

	void *ptr = malloc(size + PEAK_MALLOC_PADDING);

	return _peak_finalize_malloc(size, ptr);
}

static inline void *peak_zalloc(size_t size)
{
	void *ptr = peak_malloc(size);
	if (likely(ptr)) {
		memset(ptr, 0, size);
	}

	return ptr;
}

static inline void *_peak_realloc(void *ptr, size_t size)
{
	void *new_ptr = NULL;
	size_t old_size = 0;

	if (likely(ptr)) {
		/* tricky macros, don't reorder */
		ptr = PEAK_MALLOC_FROM_USER;
		old_size = peak_get_u64(PEAK_MALLOC_SIZE_POS);
	}

	if (likely(size)) {
		new_ptr = peak_malloc(size);
		if (unlikely(!new_ptr)) {
			return NULL;
		}

		size = old_size < size ? old_size : size;
		if (likely(size)) {
			memcpy(new_ptr, PEAK_MALLOC_TO_USER, size);
		}
	}

	free(ptr);

	return new_ptr;
}

#define peak_realloc(__ptr__, __size__)					\
	({													\
		peak_alloc_error(_peak_check_malloc(__ptr__));	\
		_peak_realloc(__ptr__, __size__);				\
	})

#define peak_reallocf(__ptr__, __size__)						\
	({															\
		void *__new_ptr__ = peak_realloc(__ptr__, __size__);	\
		if (unlikely(!__new_ptr__)) {							\
			peak_free(__ptr__);									\
		}														\
		__new_ptr__;											\
	})

static inline char *peak_strdup(const char *s1)
{
	if (unlikely(!s1)) {
		return NULL;
	}

	size_t size = strlen(s1) + 1;
	char *s2 = peak_malloc(size);
	if (likely(s2)) {
		strncpy(s2, s1, size);
	}

	return s2;
}

#define PEAK_MEMALIGN_STOP_POS		((u8 *) ptr + PEAK_CACHELINE_SIZE + size)
#define PEAK_MEMALIGN_TO_USER		((u8 *) ptr + PEAK_CACHELINE_SIZE)
#define PEAK_MEMALIGN_FROM_USER	((u8 *) ptr - PEAK_CACHELINE_SIZE)
#define PEAK_MEMALIGN_START_POS	((u8 *) ptr + PEAK_CACHELINE_SIZE - sizeof(u64))
#define PEAK_MEMALIGN_PADDING		(2 * PEAK_CACHELINE_SIZE)
#define PEAK_MEMALIGN_SIZE_POS		ptr

static inline void *_peak_finalize_memalign(const u64 size, void *ptr)
{
	if (unlikely(!ptr)) {
		return NULL;
	}

	peak_put_u64(size, PEAK_MEMALIGN_SIZE_POS);
	peak_put_u64(PEAK_MEMALIGN_VALUE, PEAK_MEMALIGN_START_POS);
	peak_put_u64(PEAK_MEMALIGN_VALUE, PEAK_MEMALIGN_STOP_POS);

	return PEAK_MEMALIGN_TO_USER;
}

static inline u32 _peak_check_memalign(void *ptr)
{
	u32 ret = PEAK_ALLOC_HEALTHY;
	u64 size;

	if (unlikely(!ptr)) {
		return ret;
	}

	/* tricky macros, don't reorder */
	ptr = PEAK_MEMALIGN_FROM_USER;
	size = peak_get_u64(PEAK_MEMALIGN_SIZE_POS);

	if (unlikely(PEAK_MEMALIGN_VALUE != peak_get_u64(PEAK_MEMALIGN_STOP_POS))) {
		ret = PEAK_ALLOC_OVERFLOW;
	}

	return ret;
}

/* TODO mac hack! */
#define memalign(__boundary__, __size__)	malloc(__size__)

static inline void *peak_malign(size_t size)
{
	if (unlikely(!size)) {
		return NULL;
	}

	void *ptr = memalign(PEAK_CACHELINE_SIZE, peak_cacheline_aligned(size) + PEAK_MEMALIGN_PADDING);

	return _peak_finalize_memalign(size, ptr);
}

static inline void *peak_zalign(size_t size)
{
	void *ptr = peak_malign(size);
	if (likely(ptr)) {
		memset(ptr, 0, size);
	}

	return ptr;
}

static inline u32 peak_check(void *ptr)
{
	u32 ret = PEAK_ALLOC_HEALTHY;

	if (unlikely(!ptr)) {
		return ret;
	}

	switch(peak_get_u64(PEAK_MALLOC_MEMALIGN_START)) {
	case PEAK_MALLOC_VALUE:
		ret = _peak_check_malloc(ptr);
		break;
	case PEAK_MEMALIGN_VALUE:
		ret = _peak_check_memalign(ptr);
		break;
	default:
		ret = PEAK_ALLOC_UNDERFLOW;
		break;
	}

	return ret;
}

static inline u32 __peak_free(void *ptr)
{
	u32 ret;

	switch(peak_get_u64(PEAK_MALLOC_MEMALIGN_START)) {
	case PEAK_MALLOC_VALUE:
		ret = _peak_check_malloc(ptr);
		if (likely(!ret)) {
			free(PEAK_MALLOC_FROM_USER);
		}
		break;
	case PEAK_MEMALIGN_VALUE:
		ret = _peak_check_memalign(ptr);
		if (likely(!ret)) {
			free(PEAK_MEMALIGN_FROM_USER);
		}
		break;
	default:
		ret = PEAK_ALLOC_UNDERFLOW;
		break;
	}

	return ret;
}

#undef PEAK_MALLOC_STOP_POS
#undef PEAK_MALLOC_TO_USER
#undef PEAK_MALLOC_FROM_USER
#undef PEAK_MALLOC_START_POS
#undef PEAK_MALLOC_PADDING
#undef PEAK_MALLOC_SIZE_POS

#undef PEAK_MEMALIGN_STOP_POS
#undef PEAK_MEMALIGN_TO_USER
#undef PEAK_MEMALIGN_FROM_USER
#undef PEAK_MEMALIGN_START_POS
#undef PEAK_MEMALIGN_PADDING
#undef PEAK_MEMALIGN_SIZE_POS

#undef PEAK_MALLOC_MEMALIGN_START

#undef PEAK_MEMALIGN_VALUE
#undef PEAK_MALLOC_VALUE

static inline u32 _peak_free(void *ptr)
{
	u32 ret = PEAK_ALLOC_HEALTHY;

	if (unlikely(!ptr)) {
		goto _peak_free_out;
	}

	ret = __peak_free(ptr);

  _peak_free_out:

	return ret;
}

#define peak_free(__ptr__)	peak_alloc_error(_peak_free(__ptr__))

#endif /* PEAK_ALLOC_H */
