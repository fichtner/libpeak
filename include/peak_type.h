#ifndef PEAK_TYPES_H
#define PEAK_TYPES_H

#ifdef __APPLE__

#include <libkern/OSAtomic.h>

typedef OSSpinLock peak_spinlock_t;

#define peak_spin_unlock	OSSpinLockUnlock
#define peak_spin_lock		OSSpinLockLock

static inline void peak_spin_init(peak_spinlock_t *lock)
{
	*lock = 0;
}

static inline void peak_spin_destroy(peak_spinlock_t *lock)
{
	(void) lock;
}

#else /* __APPLE__ */

#include <pthread.h>

typedef pthread_spinlock_t peak_spinlock_t;

#define peak_spin_unlock	pthread_spin_unlock
#define peak_spin_lock		pthread_spin_lock

static inline void peak_spin_init(peak_spinlock_t *lock)
{
	pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE);
}

static inline void peak_spin_destroy(peak_spinlock_t *lock)
{
	pthread_spin_destroy(lock);
}

#endif /* __APPLE__*/

/* this is only true for 64 bit systems */
#define s8      char
#define s16     short
#define s32     int
#define s64     long

#define u8      unsigned s8
#define u16     unsigned s16
#define u32     unsigned s32
#define u64     unsigned s64

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define peak_get_u16	peak_get_u16_le
#define peak_put_u16	peak_put_u16_le
#define peak_get_u32	peak_get_u32_le
#define peak_put_u32	peak_put_u32_le
#define peak_get_u64	peak_get_u64_le
#define peak_put_u64	peak_put_u64_le
#else
#define peak_get_u16	peak_get_u16_be
#define peak_put_u16	peak_put_u16_be
#define peak_get_u32	peak_get_u32_be
#define peak_put_u32	peak_put_u32_be
#define peak_get_u64	peak_get_u64_be
#define peak_put_u64	peak_put_u64_be
#endif

#define peak_byteswap_32(__val__)	__builtin_bswap32(__val__)
#define peak_byteswap_64(__val__)	__builtin_bswap64(__val__)

#ifdef __CHECKER__
#define __builtin_bswap32(__val__) (__val__)
#define __builtin_bswap64(__val__) (__val__)
#endif /* __CHECKER__ */

static inline u16 _peak_get_u16_le(const u8 *ptr)
{
	return ptr[0] | ptr[1] << 8;
}

static inline u16 peak_get_u16_le(const void *ptr)
{
	return _peak_get_u16_le(ptr);
}

static inline void _peak_put_u16_le(const u16 val, u8 *ptr)
{
	*ptr++ = val;
	*ptr = val >> 8;
}

static inline void peak_put_u16_le(const u16 val, void *ptr)
{
	_peak_put_u16_le(val, ptr);
}

static inline u32 _peak_get_u32_le(const u8 *ptr)
{
	return ptr[0] | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
}

static inline u32 peak_get_u32_le(const void *ptr)
{
	return _peak_get_u32_le(ptr);
}

static inline void _peak_put_u32_le(const u32 val, u8 *ptr)
{
	*ptr++ = val;
	*ptr++ = val >> 8;
	*ptr++ = val >> 16;
	*ptr = val >> 24;
}

static inline void peak_put_u32_le(const u32 val, void *ptr)
{
	_peak_put_u32_le(val, ptr);
}

static inline u64 _peak_get_u64_le(const u8 *ptr)
{
	return (u64) _peak_get_u32_le(ptr) |
		(u64) _peak_get_u32_le(ptr + 4) << 32;
}

static inline u64 peak_get_u64_le(const void *ptr)
{
	return _peak_get_u64_le(ptr);
}

static inline void _peak_put_u64_le(const u64 val, u8 *ptr)
{
	_peak_put_u32_le(val, ptr);
	_peak_put_u32_le(val >> 32, ptr + 4);
}

static inline void peak_put_u64_le(const u64 val, void *ptr)
{
	_peak_put_u64_le(val, ptr);
}

static inline u16 _peak_get_u16_be(const u8 *ptr)
{
	return ptr[0] << 8 | ptr[1];
}

static inline u16 peak_get_u16_be(const void *ptr)
{
	return _peak_get_u16_be(ptr);
}

static inline void _peak_put_u16_be(const u16 val, u8 *ptr)
{
	*ptr++ = val >> 8;
	*ptr = val;
}

static inline void peak_put_u16_be(const u16 val, void *ptr)
{
	_peak_put_u16_be(val, ptr);
}

static inline u32 _peak_get_u32_be(const u8 *ptr)
{
	return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

static inline u32 peak_get_u32_be(const void *ptr)
{
	return _peak_get_u32_be(ptr);
}

static inline void _peak_put_u32_be(const u32 val, u8 *ptr)
{
	*ptr++ = val >> 24;
	*ptr++ = val >> 16;
	*ptr++ = val >> 8;
	*ptr = val;
}

static inline void peak_put_u32_be(const u32 val, void *ptr)
{
	_peak_put_u32_be(val, ptr);
}

static inline u64 _peak_get_u64_be(const u8 *ptr)
{
	return (u64) _peak_get_u32_be(ptr) << 32 |
		(u64) _peak_get_u32_be(ptr + 4);
}

static inline u64 peak_get_u64_be(const void *ptr)
{
	return _peak_get_u64_be(ptr);
}

static inline void _peak_put_u64_be(const u64 val, u8 *ptr)
{
	_peak_put_u32_be(val >> 32, ptr);
	_peak_put_u32_be(val, ptr + 4);
}

static inline void peak_put_u64_be(const u64 val, void *ptr)
{
	_peak_put_u64_be(val, ptr);
}

#endif /* PEAK_TYPES_H */
