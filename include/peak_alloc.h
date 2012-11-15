#ifndef PEAK_ALLOC_H
#define PEAK_ALLOC_H

#include <stdlib.h>
#include <string.h>

#define CACHELINE_SIZE 64

static inline size_t
peak_cacheline_aligned(const size_t size)
{
	return (size + CACHELINE_SIZE -
	    (size % CACHELINE_SIZE ? : CACHELINE_SIZE));
}

#define ALLOC_HEALTHY		0
#define ALLOC_UNDERFLOW		1
#define ALLOC_OVERFLOW		2

#define peak_alloc_error(x) do {					\
	switch (x) {							\
	case ALLOC_HEALTHY:						\
		break;							\
	case ALLOC_UNDERFLOW:						\
		peak_panic("buffer underflow detected\n");		\
		/* NOTREACHED */					\
	case ALLOC_OVERFLOW:						\
		peak_panic("buffer overflow detected\n");		\
		/* NOTREACHED */					\
	}								\
} while (0)

struct peak_alloc_magic {
	uint64_t magic;
};

#define ALLOC_PAD(name)		(sizeof(struct peak_##name##_head) +	\
    sizeof(struct peak_alloc_magic))
#define ALLOC_MAGIC(x)							\
    (((struct peak_alloc_magic *)(x)) - 1)->magic
#define ALLOC_HEAD(name, x)	(((struct peak_##name##_head *)(x)) - 1)
#define ALLOC_USER(x)		&(x)->user
#define ALLOC_SIZE(x)		(x)->size
#define ALLOC_TAIL(x)		((struct peak_alloc_magic *)		\
    (((unsigned char *)ALLOC_USER(x)) + ALLOC_SIZE(x)))
#define ALLOC_VALUE(name)	name##_VALUE

#define ALLOC_INIT(name, x, y) do {					\
	(x)->magic = ALLOC_VALUE(name);					\
	ALLOC_SIZE(x) = y;						\
	ALLOC_TAIL(x)->magic = ALLOC_VALUE(name);			\
} while (0);

#define ALLOC_TAIL_OK(name, x)	(ALLOC_VALUE(name) == ALLOC_TAIL(h)->magic)

#define memalign_VALUE		0x8897A6B5C4D3E2F1ull
#define malloc_VALUE		0x1F2E3D4C5B6A7988ull

struct peak_malloc_head {
	uint64_t size;
	uint64_t magic;
	unsigned char user[];
};

struct peak_memalign_head {
	uint64_t size;
	uint64_t _0[6];
	uint64_t magic;
	unsigned char user[];
};

static inline void *
_peak_finalize_malloc(const uint64_t size, struct peak_malloc_head *h)
{
	if (unlikely(!h)) {
		return (NULL);
	}

	ALLOC_INIT(malloc, h, size);

	return (ALLOC_USER(h));
}

static inline unsigned int
_peak_check_malloc(void *ptr)
{
	unsigned int ret = ALLOC_HEALTHY;
	struct peak_malloc_head *h;

	if (unlikely(!ptr)) {
		return (ret);
	}

	h = ALLOC_HEAD(malloc, ptr);

	if (unlikely(!ALLOC_TAIL_OK(malloc, h))) {
		ret = ALLOC_OVERFLOW;
	}

	return (ret);
}

static inline void *
peak_malloc(size_t size)
{
	if (unlikely(!size)) {
		return (NULL);
	}

	void *ptr = malloc(size + ALLOC_PAD(malloc));

	return (_peak_finalize_malloc(size, ptr));
}

static inline void *
peak_zalloc(size_t size)
{
	void *ptr = peak_malloc(size);
	if (likely(ptr)) {
		bzero(ptr, size);
	}

	return (ptr);
}

static inline void *
_peak_realloc(void *ptr, size_t size)
{
	struct peak_malloc_head *h = NULL;
	void *new_ptr = NULL;
	size_t old_size = 0;

	if (likely(ptr)) {
		h = ALLOC_HEAD(malloc, ptr);
		old_size = h->size;
	}

	if (likely(size)) {
		new_ptr = peak_malloc(size);
		if (unlikely(!new_ptr)) {
			return (NULL);
		}

		size = old_size < size ? old_size : size;
		if (likely(size)) {
			bcopy(ALLOC_USER(h), new_ptr, size);
		}
	}

	free(h);

	return (new_ptr);
}

#define peak_realloc(x, y) ({						\
	peak_alloc_error(_peak_check_malloc(x));			\
	_peak_realloc(x, y);						\
})

#define peak_reallocf(x, y) ({						\
	void *__new__ = peak_realloc(x, y);				\
	if (unlikely(!__new__)) {					\
		peak_free(x);						\
	}								\
	__new__;							\
})

static inline char *
peak_strdup(const char *s1)
{
	if (unlikely(!s1)) {
		return (NULL);
	}

	size_t size = strlen(s1) + 1;
	char *s2 = peak_malloc(size);
	if (likely(s2)) {
		strncpy(s2, s1, size);
	}

	return (s2);
}

static inline void *
_peak_finalize_memalign(const uint64_t size, struct peak_memalign_head *h)
{
	if (unlikely(!h)) {
		return (NULL);
	}

	ALLOC_INIT(memalign, h, size);

	return (ALLOC_USER(h));
}

static inline unsigned int
_peak_check_memalign(void *ptr)
{
	unsigned int ret = ALLOC_HEALTHY;
	struct peak_memalign_head *h;

	if (unlikely(!ptr)) {
		return (ret);
	}

	h = ALLOC_HEAD(memalign, ptr);

	if (unlikely(!ALLOC_TAIL_OK(memalign, h))) {
		ret = ALLOC_OVERFLOW;
	}

	return (ret);
}

#ifndef memalign
#define memalign(x, y)	malloc(y)
#endif /* !memalign */

static inline void *
peak_malign(size_t size)
{
	if (unlikely(!size)) {
		return (NULL);
	}

	void *ptr = memalign(CACHELINE_SIZE,
	    peak_cacheline_aligned(size) + ALLOC_PAD(memalign));

	return (_peak_finalize_memalign(size, ptr));
}

static inline void *
peak_zalign(size_t size)
{
	void *ptr = peak_malign(size);
	if (likely(ptr)) {
		bzero(ptr, size);
	}

	return (ptr);
}

static inline unsigned int
peak_check(void *ptr)
{
	unsigned int ret = ALLOC_HEALTHY;

	if (unlikely(!ptr)) {
		return (ret);
	}

	switch (ALLOC_MAGIC(ptr)) {
	case ALLOC_VALUE(malloc):
		ret = _peak_check_malloc(ptr);
		break;
	case ALLOC_VALUE(memalign):
		ret = _peak_check_memalign(ptr);
		break;
	default:
		ret = ALLOC_UNDERFLOW;
		break;
	}

	return (ret);
}

static inline unsigned int
__peak_free(void *ptr)
{
	unsigned int ret;

	switch (ALLOC_MAGIC(ptr)) {
	case ALLOC_VALUE(malloc):
		ret = _peak_check_malloc(ptr);
		if (likely(!ret)) {
			free(ALLOC_HEAD(malloc, ptr));
		}
		break;
	case ALLOC_VALUE(memalign):
		ret = _peak_check_memalign(ptr);
		if (likely(!ret)) {
			free(ALLOC_HEAD(memalign, ptr));
		}
		break;
	default:
		ret = ALLOC_UNDERFLOW;
		break;
	}

	return (ret);
}

#undef memalign_VALUE
#undef malloc_VALUE

#undef ALLOC_TAIL_OK
#undef ALLOC_VALUE
#undef ALLOC_MAGIC
#undef ALLOC_USER
#undef ALLOC_SIZE
#undef ALLOC_INIT
#undef ALLOC_HEAD
#undef ALLOC_TAIL
#undef ALLOC_PAD

static inline unsigned int
_peak_free(void *ptr)
{
	unsigned int ret = ALLOC_HEALTHY;

	if (unlikely(!ptr)) {
		goto _peak_free_out;
	}

	ret = __peak_free(ptr);

  _peak_free_out:

	return (ret);
}

#define peak_free(x)	peak_alloc_error(_peak_free(x))

#endif /* !PEAK_ALLOC_H */
