/*
 * Copyright (c) 2012 Franco Fichtner <franco@packetwerk.com>
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

#include <peak.h>
#include <assert.h>

#define UNALIGNED_16_ORIG	0x1234
#define UNALIGNED_16_NEW	0x2345
#define UNALIGNED_32_ORIG	0x12345678
#define UNALIGNED_32_NEW	0x23456789
#define UNALIGNED_64_ORIG	0x123456789ABCDEF0ull
#define UNALIGNED_64_NEW	0x23456789ABCDEF01ull

#define THIS_IS_A_STRING	"Hello, world!"
#define THIS_IS_A_SHORT_STRING	"Hello!"

output_init();

static void
test_type(void)
{
	uint16_t test_val_16 = UNALIGNED_16_ORIG;
	uint32_t test_val_32 = UNALIGNED_32_ORIG;
	uint64_t test_val_64 = UNALIGNED_64_ORIG;

	le16enc(&test_val_16, test_val_16);
	assert(le16dec(&test_val_16) == bswap16(be16dec(&test_val_16)));
	be16enc(&test_val_16, bswap16(UNALIGNED_16_ORIG));
	assert(UNALIGNED_16_ORIG == test_val_16);

	le32enc(&test_val_32, test_val_32);
	assert(le32dec(&test_val_32) == bswap32(be32dec(&test_val_32)));
	be32enc(&test_val_32, bswap32(UNALIGNED_32_ORIG));
	assert(UNALIGNED_32_ORIG == test_val_32);

	le64enc(&test_val_64, test_val_64);
	assert(le64dec(&test_val_64) == bswap64(be64dec(&test_val_64)));
	be64enc(&test_val_64, bswap64(UNALIGNED_64_ORIG));
	assert(UNALIGNED_64_ORIG == test_val_64);

	test_val_16 = 0;
	assert(!wrap16(test_val_16));
	assert(wrap16(test_val_16 - 1));

	test_val_32 = 0;
	assert(!wrap32(test_val_32));
	assert(wrap32(test_val_32 - 1));

	test_val_64 = 0;
	assert(!wrap64(test_val_64));
	assert(wrap64(test_val_64 - 1));
}

static void
test_net(void)
{
	const struct netmap ip4_ref = {
		.u.byte = {
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0xff, 0xff,
			0x11, 0x22, 0x33, 0x44,
		},
	};
	const struct netmap ip6_ref = {
		.u.byte = {
			0xfe, 0x80, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x12, 0x40, 0xf3, 0xff,
			0xfe, 0xa6, 0xd7, 0x62,
		},
	};
	uint32_t ipv4_addr;
	struct netmap ip;

	be32enc(&ipv4_addr, 0x11223344);
	netmap4(&ip, ipv4_addr);
	assert(!netcmp(&ip, &ip4_ref));

	netmap6(&ip, &ip6_ref);
	assert(!netcmp(&ip, &ip6_ref));
}

struct interval {
	int left;
	int right;
};

static void
test_stash(void)
{
	struct interval test[2] = { { 2, 1 }, { 1, 2 } };
	STASH_DECLARE(stash, struct interval, 2);
	struct interval *p;
	unsigned int i = 0;

	assert(STASH_EMPTY(stash));

	STASH_PUSH(&test[0], stash);
	STASH_PUSH(&test[1], stash);

	assert(STASH_FULL(stash));

	STASH_FOREACH(p, stash) {
		assert(p->left == test[i].left &&
		    p->right == test[i].right);
		++i;
	}

	assert(STASH_COUNT(stash) == i);

	STASH_FOREACH_REVERSE(p, stash) {
		--i;
		assert(p->left == test[i].left &&
		    p->right == test[i].right);
	}

	STASH_POP(stash);
	STASH_POP(stash);

	assert(STASH_EMPTY(stash));
}

static void
test_alloc(void)
{
	uint64_t *test_ptr = peak_zalloc(sizeof(*test_ptr));

	assert(!*test_ptr);

	peak_free(test_ptr);

	assert(!peak_realloc(NULL, 0));

	test_ptr = peak_realloc(NULL, 7);
	uint64_t backup = *test_ptr;
	*test_ptr = 0;

	assert(ALLOC_OVERFLOW == _peak_mcheck(test_ptr));

	*test_ptr = backup;

	test_ptr = peak_reallocf(test_ptr, 16);

	test_ptr[0] = 'o';
	test_ptr[1] = 'k';

	assert('o' == test_ptr[0]);
	assert('k' == test_ptr[1]);

	test_ptr = peak_realloc(test_ptr, 24);

	assert('o' == test_ptr[0]);
	assert('k' == test_ptr[1]);

	test_ptr[2] = '!';

	test_ptr = peak_realloc(test_ptr, 8);

	assert('o' == test_ptr[0]);
	assert('k' != test_ptr[1]);

	assert(!peak_realloc(test_ptr, 0));

	char *test_str = peak_strdup(THIS_IS_A_STRING);

	assert(!strcmp(test_str, THIS_IS_A_STRING));

	char *test_str_mod = test_str - 1;
	char test_char = *test_str_mod;

	*test_str_mod = 0;

	assert(ALLOC_UNDERFLOW == _peak_free(test_str));

	*test_str_mod = test_char;
	test_str_mod = test_str + strlen(test_str) + 1;
	test_char = *test_str_mod;
	*test_str_mod = 0;

	assert(ALLOC_OVERFLOW == _peak_free(test_str));

	*test_str_mod = test_char;

	peak_free(test_str);

	assert(ALLOC_CACHEALIGN(ALLOC_CACHELINE + 1) ==
	    2 * ALLOC_CACHELINE);
	assert(ALLOC_CACHEALIGN(ALLOC_CACHELINE - 1) == ALLOC_CACHELINE);
	assert(ALLOC_CACHEALIGN(ALLOC_CACHELINE) == ALLOC_CACHELINE);
	assert(ALLOC_CACHEALIGN(1) == ALLOC_CACHELINE);
	assert(ALLOC_CACHEALIGN(0) == 0);

	peak_free(peak_malign(0));
	peak_free(peak_malign(1));
	peak_free(peak_zalloc(0));
	peak_free(peak_zalloc(1));

	char *test_str_aligned = peak_malign(sizeof(THIS_IS_A_STRING));

	bcopy(THIS_IS_A_STRING, test_str_aligned,
	    sizeof(THIS_IS_A_STRING));

	char *test_str_mod_aligned = test_str_aligned - 1;
	char test_char_aligned = *test_str_mod_aligned;

	*test_str_mod_aligned = 0;

	assert(ALLOC_UNDERFLOW == _peak_mcheck(test_str_aligned));

	*test_str_mod_aligned = test_char_aligned;
	test_str_mod_aligned = test_str_aligned + sizeof(THIS_IS_A_STRING);
	test_char_aligned = *test_str_mod_aligned;
	*test_str_mod_aligned = 0;

	assert(ALLOC_OVERFLOW == _peak_mcheck(test_str_aligned));

	*test_str_mod_aligned = test_char_aligned;

	assert(ALLOC_HEALTHY == _peak_mcheck(test_str_aligned));

	peak_free(test_str_aligned);
}

static void
test_prealloc(void)
{
	prealloc_t *test_mem = prealloc_initd(1, 8);
	void *test_chunk = prealloc_gets(test_mem);

	assert(test_chunk && !prealloc_gets(test_mem));

	prealloc_puts(test_mem, test_chunk);
	test_chunk = prealloc_gets(test_mem);

	assert(PREALLOC_MISSING_CHUNKS == _prealloc_exit(test_mem));

	prealloc_puts(test_mem, test_chunk);
	prealloc_exitd(test_mem);

	test_mem = prealloc_initd(8, sizeof(uint64_t));

	uint64_t *test_chunks[8];
	unsigned int i;

	for (i = 0; i < 8; ++i) {
		test_chunks[i] = prealloc_gets(test_mem);

		assert(test_chunks[i]);

		*test_chunks[i] = i;

		assert(PREALLOC_HEALTHY ==
		    _prealloc_put(test_mem, test_chunks[i]));
		assert(PREALLOC_DOUBLE_FREE ==
		    _prealloc_put(test_mem, test_chunks[i]));
		assert(prealloc_gets(test_mem) == test_chunks[i]);
	}

	assert(!prealloc_gets(test_mem));

	for (i = 0; i < 8; ++i) {
		assert(i == *test_chunks[i]);

		uint64_t magic = *(test_chunks[i] - 1);
		*(test_chunks[i] - 1) = 0;

		assert(PREALLOC_UNDERFLOW ==
		    _prealloc_put(test_mem, test_chunks[i]));

		*(test_chunks[i] - 1) = magic;

		prealloc_puts(test_mem, test_chunks[i]);
	}

	assert(PREALLOC_POOL_MISMATCH ==
	    _prealloc_put(test_mem, test_chunks));

	prealloc_exitd(test_mem);

	assert(!prealloc_initd(1, 151));
	assert(!prealloc_initd(2, ~0ULL));
}

static void
test_exalloc(void)
{
	char *s1, *s2, *s3;
	exalloc_t *mem;

	assert(!exalloc_initd(0, 8, 16, 128));
	assert(!exalloc_initd(3, 0, 16, 128));
	assert(!exalloc_initd(3, 8, 4, 128));
	assert(!exalloc_initd(3, 8, 16, 8));
	assert(!exalloc_initd(3, 16, 128, 8));

	mem = exalloc_initd(3, 8, 16, 128);

	assert(mem);
	assert(!exalloc_swap(mem, NULL, 0));

	s1 = exalloc_get(mem, sizeof(THIS_IS_A_SHORT_STRING));
	assert(!exalloc_swap(mem, s1, 129));
	s2 = exalloc_get(mem, sizeof(THIS_IS_A_SHORT_STRING));
	s3 = exalloc_get(mem, sizeof(THIS_IS_A_SHORT_STRING));
	assert(!exalloc_swap(mem, s1, sizeof(THIS_IS_A_STRING)));
	assert(s1);
	assert(s2 && s1 != s2);
	assert(s3 && s3 != s2 && s3 != s1);
	assert(!exalloc_get(mem, sizeof(THIS_IS_A_SHORT_STRING)));
	assert(!exalloc_get(mem, sizeof(THIS_IS_A_STRING)));
	s2 = exalloc_put(mem, s2);
	s3 = exalloc_put(mem, s3);
	assert(!s2 && !s3);

	s2 = s1;
	s1 = exalloc_put(mem, s1);
	assert(!s1);
	s1 = exalloc_get(mem, sizeof(THIS_IS_A_SHORT_STRING));
	assert(s1 == s2);

	bcopy(THIS_IS_A_SHORT_STRING, s1, sizeof(THIS_IS_A_SHORT_STRING));
	assert(!memcmp(s1, THIS_IS_A_SHORT_STRING,
	    sizeof(THIS_IS_A_SHORT_STRING)));

	s2 = exalloc_swap(mem, s1, sizeof(THIS_IS_A_STRING));
	assert(s2 && s2 != s1);
	assert(!memcmp(s2, THIS_IS_A_SHORT_STRING,
	    sizeof(THIS_IS_A_SHORT_STRING)));

	s1 = exalloc_swap(mem, s2, sizeof(THIS_IS_A_SHORT_STRING));
	assert(s1 && s1 != s2);
	exalloc_put(mem, s1);

	s1 = exalloc_get(mem, sizeof(THIS_IS_A_STRING));
	assert(s1);
	s2 = exalloc_swap(mem, s1, sizeof(THIS_IS_A_STRING));
	assert(s1 == s2);
	exalloc_put(mem, s2);

	exalloc_exitd(mem);
}

static void
test_output(void)
{
	output_log();
	output_bug();

	if (0) {
		/* compile only */
		panic("test");
		alert("test");
		critical("test");
		error("test");
		warning("test");
		notice("test");
		info("test");
		debug("test");
	}
}

static void
test_hash(void)
{
	ROLL_HEAD(whatever) head;
	uint32_t backup;

	assert(hash_fnv32("test", 4) == 0xAFD071E5u);
	assert(hash_fnv32(NULL, 0) == 0x811C9DC5u);

	assert(hash_joaat("test", 4) == 0x76705442u);
	assert(hash_joaat(NULL, 0) == 0x11C50567u);

	ROLL_INIT(&head, "test");
	assert(!ROLL_HASH(&head));
	ROLL_NEXT(&head);
	backup = ROLL_HASH(&head);
	assert(backup);
	ROLL_SHIFT(&head);
	assert(backup != ROLL_HASH(&head));
	ROLL_SHIFTN(&head, 2);
	assert(backup == ROLL_HASH(&head));

	ROLL_INIT(&head, "test");
	ROLL_NEXT(&head);
	ROLL_NEXT(&head);
	ROLL_NEXT(&head);
	ROLL_NEXT(&head);
	backup = ROLL_HASH(&head);

	assert(backup == hash_roll("test", 4));

	ROLL_INIT(&head, "testest");
	ROLL_NEXTN(&head, 4);
	assert(backup == ROLL_HASH(&head));
	ROLL_SHIFT(&head);
	assert(backup != ROLL_HASH(&head));
	ROLL_SHIFTN(&head, 2);
	assert(backup == ROLL_HASH(&head));

	assert(backup != hash_roll("tesT", 4));
}

static void
test_time(void)
{
	timeslice_t _timer;
	timeslice_t *timer;

	timer = &_timer;

	TIMESLICE_INIT(timer);

	TIMESLICE_NORMALISE(timer, 42);
	TIMESLICE_ADVANCE(timer, 1, 42);

	assert(timer->normal == 42);
	assert(timer->msec == 1000);

	TIMESLICE_ADVANCE(timer, 1, 45);

	assert(timer->normal == 42);
	assert(timer->msec == 1003);
}

int
main(void)
{
	pout("peak base test suite... ");

	test_type();
	test_net();
	test_stash();
	test_alloc();
	test_prealloc();
	test_exalloc();
	test_output();
	test_hash();
	test_time();

	pout("ok\n");

	return (0);
}
