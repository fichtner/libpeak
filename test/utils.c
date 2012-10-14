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
test_types(void)
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
	struct peak_tree t;
	unsigned int value;
};

static unsigned int
test_tree_lt(const void *u1, const void *u2)
{
	const struct test *t1 = u1;
	const struct test *t2 = u2;

	return (t1->value < t2->value);
}

static unsigned int
test_tree_eq(const void *u1, const void *u2)
{
	const struct test *t1 = u1;
	const struct test *t2 = u2;

	return (t1->value == t2->value);
}

static void
test_tree_simple(void)
{
	struct test *root = NULL;
	struct test t1, t2, t3;

	peak_tree_init(test_tree_lt, test_tree_eq);

	t1.value = 1;
	t2.value = 2;
	t3.value = 3;

	assert(!peak_tree_lookup(root, &t1));
	assert(!peak_tree_lookup(root, &t2));
	assert(!peak_tree_lookup(root, &t3));
	assert(!peak_tree_height(root));
	assert(!peak_tree_count(root));

	root = peak_tree_insert(root, &t1);

	assert(root == &t1);
	assert(peak_tree_height(root) == 1);
	assert(peak_tree_count(root) == 1);
	assert(&t1 == peak_tree_lookup(root, &t1));

	root = peak_tree_insert(root, &t2);

	assert(root == &t1);
	assert(root->t.t[1] == &t2.t);
	assert(peak_tree_height(root) == 2);
	assert(peak_tree_count(root) == 2);
	assert(&t1 == peak_tree_lookup(root, &t1));
	assert(&t2 == peak_tree_lookup(root, &t2));

	root = peak_tree_insert(root, &t3);

	assert(root == &t2);
	assert(root->t.t[1] == &t3.t);
	assert(root->t.t[0] == &t1.t);
	assert(peak_tree_height(root) == 2);
	assert(peak_tree_count(root) == 3);
	assert(&t1.t == peak_tree_lookup(root, &t1));
	assert(&t2.t == peak_tree_lookup(root, &t2));
	assert(&t3.t == peak_tree_lookup(root, &t3));

	root = peak_tree_remove(root, &t2);

	assert(root != &t2);
	assert(root == &t1);
	assert(root->t.t[1] == &t3.t);

	root = peak_tree_remove(root, &t1);

	assert(root != &t1);
	assert(root == &t3);

	root = peak_tree_remove(root, &t3);
	root = peak_tree_remove(root, &t3);

	assert(!root);

	root = peak_tree_insert(root, &t1);
	root = peak_tree_insert(root, &t2);
	root = peak_tree_insert(root, &t3);

	assert(peak_tree_count(root) == 3);

	root = peak_tree_collapse(root, NULL, NULL);

	assert(peak_tree_count(root) == 0);
}

static void
test_tree_free(void *ptr, void *ctx)
{
	struct peak_prealloc *mem = ctx;
	struct test *e = ptr;

	peak_preput(mem, e);
}

#define TREE_COUNT 10000

static void
test_tree_complex(void)
{
	struct test *e, *root = NULL;
	struct peak_prealloc *mem;
	struct test helper;
	unsigned int i = 1;

	mem = peak_prealloc(TREE_COUNT, sizeof(struct test));

	assert(mem);

	peak_tree_init(test_tree_lt, test_tree_eq);

	while ((e = _peak_preget(mem))) {
		assert(i - 1 == peak_tree_count(root));
		e->value = i++;
		root = peak_tree_insert(root, e);
	}

	assert(peak_tree_count(root) == TREE_COUNT);
	assert(peak_tree_height(root) == 18);
	bzero(&helper, sizeof(struct test));

	for (i = TREE_COUNT; i; --i) {
		helper.value = i;
		e = peak_tree_lookup(root, &helper);
		assert(e);
		root = peak_tree_remove(root, e);
		assert(i - 1 == peak_tree_count(root));
		_peak_preput(mem, e);
	}

	for (i = 1; i < 5; --i) {
		e = _peak_preget(mem);
		e->value = i;
		root = peak_tree_insert(root, e);
	}

	peak_tree_collapse(root, test_tree_free, mem);

	peak_prefree(mem);
}

UNITTEST int
main(void)
{
	peak_log(LOG_EMERG, "peak utilities test suite... ");

	test_types();
	test_alloc();
	test_prealloc();
	test_output();
	test_hash();
	test_tree_simple();
	test_tree_complex();

	peak_log(LOG_EMERG, "ok\n");

	return (0);
}
