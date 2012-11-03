#include <peak.h>
#include <assert.h>

#define UNALIGNED_16_ORIG	0x1234
#define UNALIGNED_16_NEW	0x2345
#define UNALIGNED_32_ORIG	0x12345678
#define UNALIGNED_32_NEW	0x23456789
#define UNALIGNED_64_ORIG	0x123456789ABCDEF0ull
#define UNALIGNED_64_NEW	0x23456789ABCDEF01ull

#define THIS_IS_A_STRING	"Hello, world!"

peak_priority_init();

static void
test_type(void)
{
	uint16_t test_val_16 = UNALIGNED_16_ORIG;
	uint64_t test_val_64 = UNALIGNED_64_ORIG;
	uint32_t test_val_32 = UNALIGNED_32_ORIG;

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

	assert(!peak_uint16_wrap(test_val_16 = 0));
	assert(peak_uint16_wrap(test_val_16 - 1));
	assert(!peak_uint32_wrap(test_val_32 = 0));
	assert(peak_uint32_wrap(test_val_32 - 1));
	assert(!peak_uint64_wrap(test_val_64 = 0));
	assert(peak_uint64_wrap(test_val_64 - 1));
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
	assert(PEAK_CACHELINE_SIZE ==
	    peak_cacheline_aligned(PEAK_CACHELINE_SIZE - 1));
	assert(PEAK_CACHELINE_SIZE ==
	    peak_cacheline_aligned(PEAK_CACHELINE_SIZE));
	assert(PEAK_CACHELINE_SIZE == peak_cacheline_aligned(1));
	assert(2 * PEAK_CACHELINE_SIZE ==
	    peak_cacheline_aligned(PEAK_CACHELINE_SIZE + 1));

	peak_free(peak_zalign(0));
	peak_free(peak_zalign(1));
	peak_free(peak_zalloc(0));
	peak_free(peak_zalloc(1));

	char *test_str_aligned = peak_malign(sizeof(THIS_IS_A_STRING));

	memcpy(test_str_aligned, THIS_IS_A_STRING,
	    sizeof(THIS_IS_A_STRING));

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

static void
test_prealloc(void)
{
	struct peak_prealloc *test_mem = peak_prealloc(1, 8);
	void *test_chunk = peak_preget(test_mem);

	assert(test_chunk && !peak_preget(test_mem));

	peak_preput(test_mem, test_chunk);
	test_chunk = peak_preget(test_mem);

	assert(PEAK_PREALLOC_MISSING_CHUNKS == _peak_prefree(test_mem));

	peak_preput(test_mem, test_chunk);
	peak_prefree(test_mem);

	test_mem = peak_prealloc(8, sizeof(uint64_t));

	uint64_t *test_chunks[8];
	unsigned int i;

	for (i = 0; i < 8; ++i) {
		test_chunks[i] = peak_preget(test_mem);

		assert(test_chunks[i]);

		*test_chunks[i] = i;

		assert(PEAK_PREALLOC_HEALTHY ==
		    __peak_preput(test_mem, test_chunks[i]));
		assert(PEAK_PREALLOC_DOUBLE_FREE ==
		    __peak_preput(test_mem, test_chunks[i]));
		assert(peak_preget(test_mem) == test_chunks[i]);
	}

	assert(!peak_preget(test_mem));

	for (i = 0; i < 8; ++i) {
		assert(i == *test_chunks[i]);

		uint64_t magic = *(test_chunks[i] - 1);
		*(test_chunks[i] - 1) = 0;

		assert(PEAK_PREALLOC_UNDERFLOW ==
		    __peak_preput(test_mem, test_chunks[i]));

		*(test_chunks[i] - 1) = magic;

		peak_preput(test_mem, test_chunks[i]);
	}

	peak_prefree(test_mem);

	assert(!peak_prealloc(1, 151));
	assert(!peak_prealloc(2, ~0ULL));
}

static void
test_output(void)
{
	peak_priority_log();
	peak_priority_bug();

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

static void
test_hash(void)
{
	assert(peak_hash_fnv32("test", 4) == 0xAFD071E5u);
	assert(peak_hash_fnv32(NULL, 0) == 0x811C9DC5u);
}

struct test {
	AA_ENTRY(test) t;
	unsigned int value;
};

#define test_tree_lt(t1, t2)	((t1)->value < (t2)->value)
#define test_tree_eq(t1, t2)	((t1)->value == (t2)->value)

AA_GENERATE_STATIC(, test, t, test_tree_lt, test_tree_eq);

static void
test_tree_simple(void)
{
	struct test t1, t2, t3;
	AA_HEAD(test) root;

	AA_INIT(, &root, t);

	t1.value = 1;
	t2.value = 2;
	t3.value = 3;

	assert(!AA_FIND(, &root, &t1));
	assert(!AA_FIND(, &root, &t2));
	assert(!AA_FIND(, &root, &t3));
	assert(!AA_HEIGHT(, &root));
	assert(!AA_COUNT(, &root));
	assert(!AA_MIN(, &root));
	assert(!AA_MAX(, &root));

	AA_INSERT(, &root, &t1);

	assert(AA_ROOT(&root) == &t1);
	assert(AA_HEIGHT(, &root) == 1);
	assert(AA_COUNT(, &root) == 1);
	assert(AA_FIND(, &root, &t1) == &t1);
	assert(AA_MIN(, &root) == &t1);
	assert(AA_MAX(, &root) == &t1);

	AA_INSERT(, &root, &t2);

	assert(AA_ROOT(&root) == &t1);
	assert(AA_RIGHT(root, t) == &t2);
	assert(AA_HEIGHT(, &root) == 2);
	assert(AA_COUNT(, &root) == 2);
	assert(AA_FIND(, &root, &t1) == &t1);
	assert(AA_FIND(, &root, &t2) == &t2);
	assert(AA_MIN(, &root) == &t1);
	assert(AA_MAX(, &root) == &t2);

	AA_INSERT(, &root, &t3);

	assert(AA_ROOT(&root) == &t2);
	assert(AA_RIGHT(root, t) == &t3);
	assert(AA_LEFT(root, t) == &t1);
	assert(AA_HEIGHT(, &root) == 2);
	assert(AA_COUNT(, &root) == 3);
	assert(AA_FIND(, &root, &t1) == &t1);
	assert(AA_FIND(, &root, &t2) == &t2);
	assert(AA_FIND(, &root, &t3) == &t3);
	assert(AA_MIN(, &root) == &t1);
	assert(AA_MAX(, &root) == &t3);

	AA_REMOVE(, &root, &t2);

	assert(AA_ROOT(&root) != &t2);
	assert(AA_ROOT(&root) == &t1);
	assert(AA_RIGHT(root, t) == &t3);

	AA_REMOVE(, &root, &t1);

	assert(AA_ROOT(&root) != &t1);
	assert(AA_ROOT(&root) == &t3);

	AA_REMOVE(, &root, &t3);
	AA_REMOVE(, &root, &t3);

	assert(AA_EMPTY(&root));

	AA_INSERT(, &root, &t1);
	AA_INSERT(, &root, &t2);
	AA_INSERT(, &root, &t3);

	assert(AA_COUNT(, &root) == 3);

	AA_COLLAPSE(, &root, NULL, NULL);

	assert(AA_COUNT(, &root) == 0);
	assert(AA_EMPTY(&root));
}

static void
test_tree_free(struct test *elm, void *ctx)
{
	struct peak_prealloc *mem = ctx;

	peak_preput(mem, elm);
}

#define TREE_COUNT 10000

static void
test_tree_complex(void)
{
	struct peak_prealloc *mem;
	struct test helper;
	unsigned int i = 1;
	AA_HEAD(test) root;
	struct test *e;

	mem = peak_prealloc(TREE_COUNT, sizeof(struct test));

	assert(mem);

	AA_INIT(, &root, t);

	while ((e = _peak_preget(mem))) {
		assert(i - 1 == AA_COUNT(, &root));
		e->value = i++;
		AA_INSERT(, &root, e);
	}

	assert(AA_COUNT(, &root) == TREE_COUNT);
	assert(AA_HEIGHT(, &root) == 18);
	bzero(&helper, sizeof(struct test));

	for (i = TREE_COUNT; i; --i) {
		helper.value = i;
		e = AA_FIND(, &root, &helper);
		assert(e);
		AA_REMOVE(, &root, e);
		assert(i - 1 == AA_COUNT(, &root));
		_peak_preput(mem, e);
	}

	for (i = 1; i < 5; --i) {
		e = _peak_preget(mem);
		e->value = i;
		AA_INSERT(, &root, e);
	}

	AA_COLLAPSE(, &root, test_tree_free, mem);

	peak_prefree(mem);
}

static void
test_tree_fringe(void)
{
	struct test t1, t2, t3;
	AA_HEAD(test) root;

	AA_INIT(, &root, t);

	t1.t.aae_child[0] = t1.t.aae_child[1] = NIL();
	t2.t.aae_child[0] = t2.t.aae_child[1] = NIL();
	t3.t.aae_child[0] = t3.t.aae_child[1] = NIL();

	assert(!AA_COUNT(, &root));
	assert(!AA_HEIGHT(, &root));

	AA_ROOT(&root) = &t1;

	assert(AA_COUNT(, &root) == 1);
	assert(AA_HEIGHT(, &root) == 1);

	t1.t.aae_child[1] = &t2;

	assert(AA_COUNT(, &root) == 2);
	assert(AA_HEIGHT(, &root) == 2);

	t1.t.aae_child[0] = &t3;

	assert(AA_COUNT(, &root) == 3);
	assert(AA_HEIGHT(, &root) == 2);

	t1.t.aae_child[0] = NIL();
	t2.t.aae_child[1] = &t3;

	assert(AA_COUNT(, &root) == 3);
	assert(AA_HEIGHT(, &root) == 3);

	t2.t.aae_child[1] = NIL();
	t2.t.aae_child[0] = &t3;

	assert(AA_COUNT(, &root) == 3);
	assert(AA_HEIGHT(, &root) == 3);

	t1.t.aae_child[1] = NIL();
	t1.t.aae_child[0] = &t2;

	assert(AA_COUNT(, &root) == 3);
	assert(AA_HEIGHT(, &root) == 3);

	t2.t.aae_child[0] = NIL();
	t2.t.aae_child[1] = &t3;

	assert(AA_COUNT(, &root) == 3);
	assert(AA_HEIGHT(, &root) == 3);
}

UNITTEST int
main(void)
{
	peak_log(LOG_EMERG, "peak base test suite... ");

	test_type();
	test_alloc();
	test_prealloc();
	test_output();
	test_hash();
	test_tree_simple();
	test_tree_complex();
	test_tree_fringe();

	peak_log(LOG_EMERG, "ok\n");

	return (0);
}
