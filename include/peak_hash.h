#ifndef PEAK_HASH_H
#define PEAK_HASH_H

#define FNV_OFFSET_32	2166136261u
#define FNV_PRIME_32	16777619u

static inline uint32_t
_peak_hash_fnv32(const uint8_t *buf, const uint32_t len)
{
    uint32_t i, h = FNV_OFFSET_32;

	for (i = 0; i < len; ++i) {
		/* this is version 1A actually */
		h = FNV_PRIME_32 * (h ^ buf[i]);
	}

	return (h);
}

#undef FNV_OFFSET_32
#undef FNV_PRIME_32

static inline uint32_t
peak_hash_fnv32(const void *buf, const uint32_t len)
{
	return (_peak_hash_fnv32(buf, len));
}

#endif /* !PEAK_HASH_H */
