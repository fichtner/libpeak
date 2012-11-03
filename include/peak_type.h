#ifndef PEAK_TYPE_H
#define PEAK_TYPE_H

#include <pthread.h>
#include <stdint.h>

#ifdef __APPLE__

#include <libkern/OSAtomic.h>

typedef OSSpinLock peak_spinlock_t;

#define peak_spin_unlock	OSSpinLockUnlock
#define peak_spin_lock		OSSpinLockLock

static inline void
peak_spin_init(peak_spinlock_t *lock)
{
	*lock = 0;
}

static inline void
peak_spin_exit(peak_spinlock_t *lock)
{
	(void) lock;
}

#define PTHREAD_BARRIER_SERIAL_THREAD -1

#else /* !__APPLE__ */

typedef pthread_spinlock_t peak_spinlock_t;

#define peak_spin_unlock	pthread_spin_unlock
#define peak_spin_lock		pthread_spin_lock

static inline void
peak_spin_init(peak_spinlock_t *lock)
{
	pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE);
}

static inline void
peak_spin_exit(peak_spinlock_t *lock)
{
	pthread_spin_destroy(lock);
}

#endif /* __APPLE__*/

#define PEAK_BARRIER_SERIAL_THREAD PTHREAD_BARRIER_SERIAL_THREAD

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned int count, threads;
} peak_barrier_t;

static inline int
peak_barrier_init(peak_barrier_t *barrier)
{
	pthread_mutex_init(&barrier->mutex, NULL);
	pthread_cond_init(&barrier->cond, NULL);
	barrier->threads = 0U - 1U;
	barrier->count = 0;

	return (0);
}

static inline int
peak_barrier_exit(peak_barrier_t *barrier)
{
	pthread_mutex_destroy(&barrier->mutex);
	pthread_cond_destroy(&barrier->cond);
	barrier->threads = 0;
	barrier->count = 0;

	return (0);
}

static inline int
peak_barrier_wait(peak_barrier_t *barrier)
{
	int ret = 0;

	pthread_mutex_lock(&barrier->mutex);
	if (barrier->count < barrier->threads) {
		++barrier->count;
		pthread_cond_wait(&barrier->cond, &barrier->mutex);
	} else {
		ret = PEAK_BARRIER_SERIAL_THREAD;
	}
	pthread_mutex_unlock(&barrier->mutex);

	return (ret);
}

static inline int
peak_barrier_wake(peak_barrier_t *barrier)
{
	pthread_mutex_lock(&barrier->mutex);
	if (barrier->count == barrier->threads) {
		pthread_cond_broadcast(&barrier->cond);
		barrier->count = 0;
	}
	pthread_mutex_unlock(&barrier->mutex);

	return (0);
}

static inline int
peak_barrier_join(peak_barrier_t *barrier)
{
	int ret = 0;

	pthread_mutex_lock(&barrier->mutex);
	if (!barrier->count) {
		++barrier->threads;
	} else {
		ret = 1;
	}
	pthread_mutex_unlock(&barrier->mutex);

	return (ret);
}

static inline int
peak_barrier_leave(peak_barrier_t *barrier)
{
	int ret = 0;

	pthread_mutex_lock(&barrier->mutex);
	if (!barrier->count) {
		--barrier->threads;
	} else {
		ret = 1;
	}
	pthread_mutex_unlock(&barrier->mutex);

	return (ret);
}

static inline uint16_t
bswap16(uint16_t u)
{
	return ((u << 8) | (u >> 8));
}

#define bswap32(__val__)	__builtin_bswap32(__val__)
#define bswap64(__val__)	__builtin_bswap64(__val__)

#ifdef __CHECKER__
#define __sync_nand_and_fetch(__ptr__, __val__)	(__val__)
#define __sync_fetch_and_nand(__ptr__, __val__)	(__val__)
#define __sync_and_and_fetch(__ptr__, __val__)	(__val__)
#define __sync_fetch_and_and(__ptr__, __val__)	(__val__)
#define __sync_add_and_fetch(__ptr__, __val__)	(__val__)
#define __sync_fetch_and_add(__ptr__, __val__)	(__val__)
#define __sync_sub_and_fetch(__ptr__, __val__)	(__val__)
#define __sync_fetch_and_sub(__ptr__, __val__)	(__val__)
#define __sync_xor_and_fetch(__ptr__, __val__)	(__val__)
#define __sync_fetch_and_xor(__ptr__, __val__)	(__val__)
#define __sync_or_and_fetch(__ptr__, __val__)	(__val__)
#define __sync_fetch_and_or(__ptr__, __val__)	(__val__)
#define __builtin_bswap32(__val__)		(__val__)
#define __builtin_bswap64(__val__)		(__val__)
#endif /* __CHECKER__ */

static inline uint16_t
le16dec(const void *pp)
{
	const unsigned char *p = pp;

	return (p[1] << 8 | p[0]);
}

static inline void
le16enc(void *pp, const uint16_t u)
{
	unsigned char *p = pp;

	p[0] = u;
	p[1] = u >> 8;
}

static inline uint32_t
le32dec(const void *pp)
{
	const unsigned char *p = pp;

	return ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

static inline void
le32enc(void *pp, const uint32_t u)
{
	unsigned char *p = pp;

	p[0] = u;
	p[1] = u >> 8;
	p[2] = u >> 16;
	p[3] = u >> 24;
}

static inline uint64_t
le64dec(const void *pp)
{
	const unsigned char *p = pp;

	return (((uint64_t)le32dec(p + 4) << 32) | le32dec(p));
}

static inline void
le64enc(void *pp, const uint64_t u)
{
	unsigned char *p = pp;

	le32enc(p, u);
	le32enc(p + 4, u >> 32);
}

static inline uint16_t
be16dec(const void *pp)
{
	const unsigned char *p = pp;

	return (p[1] | (p[0] << 8));
}

static inline void
be16enc(void *pp, const uint16_t u)
{
	unsigned char *p = pp;

	p[0] = u >> 8;
	p[1] = u;
}

static inline uint32_t
be32dec(const void *pp)
{
	const unsigned char *p = pp;

	return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

static inline void
be32enc(void *pp, const uint32_t u)
{
	unsigned char *p = pp;

	p[0] = u >> 24;
	p[1] = u >> 16;
	p[2] = u >> 8;
	p[3] = u;
}

static inline uint64_t
be64dec(const void *pp)
{
	const unsigned char *p = pp;

	return (((uint64_t)be32dec(p) << 32) | be32dec(p + 4));
}

static inline void
be64enc(void *pp, const uint64_t u)
{
	unsigned char *p = pp;

	be32enc(p, u >> 32);
	be32enc(p + 4, u);
}

#define peak_uint16_wrap(__x__) ((uint16_t)(__x__) > UINT16_MAX / 2U)
#define peak_uint32_wrap(__x__) ((uint32_t)(__x__) > UINT32_MAX / 2U)
#define peak_uint64_wrap(__x__) ((uint64_t)(__x__) > UINT64_MAX / 2ULL)

#endif /* !PEAK_TYPE_H */
