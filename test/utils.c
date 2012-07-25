#include "peak_type.h"
#include "peak_macro.h"
#include "peak_output.h"
#include "peak_alloc.h"
#include "peak_prealloc.h"
#include "peak_tree.h"

#include <assert.h>

#define UNALIGNED_16_ORIG	0x1234
#define UNALIGNED_16_NEW	0x2345
#define UNALIGNED_32_ORIG	0x12345678
#define UNALIGNED_32_NEW	0x23456789
#define UNALIGNED_64_ORIG	0x123456789ABCDEF0
#define UNALIGNED_64_NEW	0x23456789ABCDEF01

#define THIS_IS_A_STRING	"Hello, world!"

int _peak_bug_priority = peak_priority_init();
int _peak_log_priority = peak_priority_init();

static void test_types(void)
{
	assert(sizeof(s8) == 1);
	assert(sizeof(u8) == 1);
	assert(sizeof(s16) == 2);
	assert(sizeof(u16) == 2);
	assert(sizeof(s32) == 4);
	assert(sizeof(u32) == 4);
	assert(sizeof(s64) == 8);
	assert(sizeof(u64) == 8);

	u16 test_val_16 = UNALIGNED_16_ORIG;

	assert(UNALIGNED_16_ORIG == peak_get_u16(&test_val_16));

	peak_put_u16(UNALIGNED_16_NEW, &test_val_16);

	assert(UNALIGNED_16_NEW == test_val_16);

	peak_put_u16_le(test_val_16, &test_val_16);

	assert(test_val_16 == peak_get_u16_le(&test_val_16));

	u32 test_val_32 = UNALIGNED_32_ORIG;

	assert(UNALIGNED_32_ORIG == peak_get_u32(&test_val_32));

	peak_put_u32(UNALIGNED_32_NEW, &test_val_32);

	assert(UNALIGNED_32_NEW == test_val_32);

	peak_put_u32_le(test_val_32, &test_val_32);

	assert(peak_get_u32_le(&test_val_32) == peak_byteswap_32(peak_get_u32_be(&test_val_32)));

	u64 test_val_64 = UNALIGNED_64_ORIG;

	assert(UNALIGNED_64_ORIG == peak_get_u64(&test_val_64));

	peak_put_u64(UNALIGNED_64_NEW, &test_val_64);

	assert(UNALIGNED_64_NEW == test_val_64);

	peak_put_u64_le(test_val_64, &test_val_64);

	assert(peak_get_u64_le(&test_val_64) == peak_byteswap_64(peak_get_u64_be(&test_val_64)));
}

static void test_alloc(void)
{
	u64 *test_ptr = peak_zalloc(sizeof(*test_ptr));

	assert(!peak_get_u64(test_ptr));

	peak_free(test_ptr);

	assert(!peak_realloc(NULL, 0));

	test_ptr = peak_realloc(NULL, 7);
	u64 backup = *test_ptr;
	*test_ptr = 0;

	assert(PEAK_ALLOC_OVERFLOW == peak_check(test_ptr));

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

	assert(PEAK_ALLOC_UNDERFLOW == _peak_free(test_str));

	*test_str_mod = test_char;
	test_str_mod = test_str + strlen(test_str) + 1;
	test_char = *test_str_mod;
	*test_str_mod = 0;

	assert(PEAK_ALLOC_OVERFLOW == _peak_free(test_str));

	*test_str_mod = test_char;

	peak_free(test_str);

	assert(0 == peak_cacheline_aligned(0));

	assert(PEAK_CACHELINE_SIZE == peak_cacheline_aligned(PEAK_CACHELINE_SIZE - 1));
	assert(PEAK_CACHELINE_SIZE == peak_cacheline_aligned(PEAK_CACHELINE_SIZE));
	assert(PEAK_CACHELINE_SIZE == peak_cacheline_aligned(1));

	assert(2 * PEAK_CACHELINE_SIZE == peak_cacheline_aligned(PEAK_CACHELINE_SIZE + 1));

	peak_free(peak_zalign(0));
	peak_free(peak_zalign(1));
	peak_free(peak_zalloc(0));
	peak_free(peak_zalloc(1));

	char *test_str_aligned = peak_malign(sizeof(THIS_IS_A_STRING));

	memcpy(test_str_aligned, THIS_IS_A_STRING, sizeof(THIS_IS_A_STRING));

	char *test_str_mod_aligned = test_str_aligned - 1;
	char test_char_aligned = *test_str_mod_aligned;

	*test_str_mod_aligned = 0;

	assert(PEAK_ALLOC_UNDERFLOW == peak_check(test_str_aligned));

	*test_str_mod_aligned = test_char_aligned;
	test_str_mod_aligned = test_str_aligned + sizeof(THIS_IS_A_STRING);
	test_char_aligned = *test_str_mod_aligned;
	*test_str_mod_aligned = 0;

	assert(PEAK_ALLOC_OVERFLOW == peak_check(test_str_aligned));

	*test_str_mod_aligned = test_char_aligned;

	assert(PEAK_ALLOC_HEALTHY == peak_check(test_str_aligned));

	peak_free(test_str_aligned);
}

static void test_prealloc(void)
{
	struct peak_prealloc_struct *test_mem = peak_prealloc(1, 8);
	void *test_chunk = peak_preget(test_mem);

	assert(test_chunk && !peak_preget(test_mem));

	peak_preput(test_mem, test_chunk);
	test_chunk = peak_preget(test_mem);

	assert(PEAK_PREALLOC_MISSING_CHUNKS == _peak_prefree(test_mem));

	peak_preput(test_mem, test_chunk);
	peak_prefree(test_mem);

	test_mem = peak_prealloc(8, sizeof(u64));

	u64 *test_chunks[8];
	u32 i;

	for (i = 0; i < 8; ++i) {
		test_chunks[i] = peak_preget(test_mem);

		assert(test_chunks[i]);

		*test_chunks[i] = i;

		assert(PEAK_PREALLOC_HEALTHY == __peak_preput(test_mem, test_chunks[i]));
		assert(PEAK_PREALLOC_DOUBLE_FREE == __peak_preput(test_mem, test_chunks[i]));
		assert(peak_preget(test_mem) == test_chunks[i]);
	}

	assert(!peak_preget(test_mem));

	for (i = 0; i < 8; ++i) {
		assert(i == *test_chunks[i]);

		u64 magic = *(test_chunks[i] - 1);
		*(test_chunks[i] - 1) = 0;

		assert(PEAK_PREALLOC_UNDERFLOW == __peak_preput(test_mem, test_chunks[i]));

		*(test_chunks[i] - 1) = magic;

		peak_preput(test_mem, test_chunks[i]);
	}

	peak_prefree(test_mem);

	assert(!peak_prealloc(1, 151));
	assert(!peak_prealloc(2, ~0ULL));
}

static void test_output(void)
{
	peak_priority_bump(_peak_log_priority);
	peak_priority_bump(_peak_bug_priority);

	if (0) {
		/* compile only */
		peak_panic("test");
		peak_alert("test");
		peak_crit("test");
		peak_error("test");
		peak_warn("test");
		peak_notice("test");
		peak_info("test");
		peak_debug("test");
	}
}

struct test {
	struct peak_tree t;
	u32 value;
};

static u32 test_tree_lt(const void *u1, const void *u2)
{
	const struct test *t1 = u1;
	const struct test *t2 = u2;

	return (t1->value < t2->value);
}

static u32 test_tree_eq(const void *u1, const void *u2)
{
	const struct test *t1 = u1;
	const struct test *t2 = u2;

	return (t1->value == t2->value);
}

static void test_tree_simple(void)
{
	struct peak_tree *root = NIL;
	struct test t1, t2, t3;

	peak_tree_init(test_tree_lt, test_tree_eq);

	t1.value = 1;
	t2.value = 2;
	t3.value = 3;

	assert(NIL == peak_tree_lookup(root, &t1));
	assert(NIL == peak_tree_lookup(root, &t2));
	assert(NIL == peak_tree_lookup(root, &t3));

	root = peak_tree_insert(root, &t1);

	assert(root == &t1.t);
	assert(&t1.t == peak_tree_lookup(root, &t1));

	root = peak_tree_insert(root, &t2);

	assert(root == &t1.t);
	assert(root->t[1] == &t2.t);
	assert(&t1.t == peak_tree_lookup(root, &t1));
	assert(&t2.t == peak_tree_lookup(root, &t2));

	root = peak_tree_insert(root, &t3);

	assert(root == &t2.t);
	assert(root->t[1] == &t3.t);
	assert(root->t[0] == &t1.t);
	assert(&t1.t == peak_tree_lookup(root, &t1));
	assert(&t2.t == peak_tree_lookup(root, &t2));
	assert(&t3.t == peak_tree_lookup(root, &t3));

	root = peak_tree_remove(root, &t2);

	assert(root != &t2.t);
	assert(root == &t1.t);
	assert(root->t[0] == NIL);
	assert(root->t[1] == &t3.t);

	root = peak_tree_remove(root, &t1);

	assert(root != &t1.t);
	assert(root == &t3.t);
	assert(root->t[0] == NIL);
	assert(root->t[1] == NIL);

	root = peak_tree_remove(root, &t3);
	root = peak_tree_remove(root, &t3);

	assert(root == NIL);
}

#define TREE_COUNT 10000

static void test_tree_complex(void)
{
	struct peak_prealloc_struct *mem;
	struct peak_tree *root = NIL;
	struct test helper;
	struct test *e;
	u32 i = 1;

	mem = peak_prealloc(TREE_COUNT, sizeof(struct test));

	assert(mem);

	peak_tree_init(test_tree_lt, test_tree_eq);

	while ((e = _peak_preget(mem))) {
		assert(i - 1 == peak_tree_count(root));
		e->value = i++;
		root = peak_tree_insert(root, e);
	}

	assert(TREE_COUNT == peak_tree_count(root));
	assert(18 == peak_tree_height(root));
	memset(&helper, 0, sizeof(struct test));

	for (i = TREE_COUNT; i; --i) {
		helper.value = i;
		e = peak_tree_lookup(root, &helper);
		assert(e);
		root = peak_tree_remove(root, e);
		assert(e->t.t[0] == NIL);
		assert(e->t.t[1] == NIL);
		assert(i - 1 == peak_tree_count(root));
		_peak_preput(mem, e);
	}

	peak_prefree(mem);
}

int main(void)
{
	peak_log(LOG_EMERG, "peak utilities test suite... ");

	test_types();
	test_alloc();
	test_prealloc();
	test_output();
	test_tree_simple();
	test_tree_complex();

	peak_log(LOG_EMERG, "ok\n");

	return 0;
}
