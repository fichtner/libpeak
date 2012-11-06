#ifndef PEAK_SYS_H
#define PEAK_SYS_H

#include <pthread.h>

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
	unsigned int threads;
	unsigned int count;
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

#endif /* !PEAK_SYS_H */
