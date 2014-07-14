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

#ifndef PEAK_SYS_H
#define PEAK_SYS_H

#include <pthread.h>

#ifdef __APPLE__

#include <libkern/OSAtomic.h>

typedef OSSpinLock spinlock_t;

#define spin_unlock	OSSpinLockUnlock
#define spin_lock	OSSpinLockLock

static inline void
spin_init(spinlock_t *lock)
{
	*lock = 0;
}

static inline void
spin_exit(spinlock_t *lock)
{
	(void) lock;
}

#else /* !__APPLE__ */

typedef pthread_spinlock_t spinlock_t;

#define spin_unlock	pthread_spin_unlock
#define spin_lock	pthread_spin_lock

static inline void
spin_init(spinlock_t *lock)
{
	pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE);
}

static inline void
spin_exit(spinlock_t *lock)
{
	pthread_spin_destroy(lock);
}

#endif /* __APPLE__*/

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned int threads;
	unsigned int count;
} barrier_t;

static inline int
barrier_init(barrier_t *barrier)
{
	pthread_mutex_init(&barrier->mutex, NULL);
	pthread_cond_init(&barrier->cond, NULL);
	barrier->threads = 0U - 1U;
	barrier->count = 0;

	return (0);
}

static inline int
barrier_exit(barrier_t *barrier)
{
	pthread_mutex_destroy(&barrier->mutex);
	pthread_cond_destroy(&barrier->cond);
	barrier->threads = 0;
	barrier->count = 0;

	return (0);
}

static inline int
barrier_wait(barrier_t *barrier)
{
	int ret = 0;

	pthread_mutex_lock(&barrier->mutex);
	if (barrier->count < barrier->threads) {
		++barrier->count;
		pthread_cond_wait(&barrier->cond, &barrier->mutex);
	} else {
		/* return number of participating threads */
		ret = barrier->threads + 1;
	}
	pthread_mutex_unlock(&barrier->mutex);

	return (ret);
}

static inline int
barrier_wake(barrier_t *barrier)
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
barrier_join(barrier_t *barrier)
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
barrier_leave(barrier_t *barrier)
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
