/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
 * Copyright (c) 2014 Masoud Chelongar <masoud@packetwerk.com>
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

output_init();

static void
test_string_single(void)
{
	STASH_DECLARE(stash, unsigned int, 10);
	struct peak_strings *root;
	unsigned int *ret;

	peak_string_exit(peak_string_init());

	root = peak_string_init();
	assert(root);

	/* finds nothing */
	peak_string_find(root, "test", 4, stash);
	assert(STASH_EMPTY(stash));

	/* deduplicate strings */
	assert(1 == peak_string_add(root, 1, "test", 4, STRING_RAW));
	assert(1 == peak_string_add(root, 2, "test", 4, STRING_RAW));

	/* finds nothing */
	peak_string_find(root, NULL, 0, stash);
	assert(STASH_EMPTY(stash));
	peak_string_find(root, "test", 3, stash);
	assert(STASH_EMPTY(stash));

	/* single match */
	peak_string_find(root, "test", 4, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	/* single match over zeroed bytes */
	peak_string_find(root, "oO\0test", 7, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	/* multiple (sub)matches */
	peak_string_find(root, "test testest tesT tes", 21, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_string_exit(NULL);
	peak_string_exit(root);
}

static void
test_string_fi(void)
{
	STASH_DECLARE(stash, unsigned int, 10);
	struct peak_strings *root;
	unsigned int *ret;

	root = peak_string_init();
	assert(root);

	assert(1 == peak_string_add(root, 1, "test", 4, STRING_FI));

	/* finds nothing */
	peak_string_find(root, "string test", 11, stash);
	assert(STASH_EMPTY(stash));

	/* single match */
	peak_string_find(root, "test string", 11, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_string_exit(root);
}

static void
test_string_wildcard(void)
{
	STASH_DECLARE(stash, unsigned int, 10);
	struct peak_strings *root;
	unsigned int *ret;

	root = peak_string_init();
	assert(root);

	assert(1 == peak_string_add(root, 1, "t?st", 4, STRING_RAW));

	/* finds nothing */
	peak_string_find(root, "string tst", 10, stash);
	assert(STASH_EMPTY(stash));

	/* single match */
	peak_string_find(root, "test string", 11, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_string_exit(root);
}

static void
test_string_la(void)
{
	STASH_DECLARE(stash, unsigned int, 10);
	struct peak_strings *root;
	unsigned int *ret;

	root = peak_string_init();
	assert(root);

	assert(1 == peak_string_add(root, 1, "test", 4, STRING_LA));

	/* finds nothing */
	peak_string_find(root, "test string", 11, stash);
	assert(STASH_EMPTY(stash));

	/* single match */
	peak_string_find(root, "string test", 11, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_string_exit(root);
}

static void
test_string_fila(void)
{
	STASH_DECLARE(stash, unsigned int, 10);
	struct peak_strings *root;
	unsigned int *ret;

	root = peak_string_init();
	assert(root);

	assert(1 == peak_string_add(root, 1, "test", 4, STRING_FILA));

	/* finds nothing */
	peak_string_find(root, "test string test", 16, stash);
	assert(STASH_EMPTY(stash));

	/* single match */
	peak_string_find(root, "test", 4, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_string_exit(root);
}

static void
test_string_multiple(void)
{
	const char *text = "Lorem ipsum dolor sit amet, "
	    "consetetur sadipscing elitr, sed diam nonumy "
	    "eirmod tempor invidunt ut labore et dolore "
	    "magna aliquyam erat, sed diam voluptua.";
	const unsigned int result[] = { 1, 6, 2, 5, 2, 5, 4 };
	STASH_DECLARE(stash, unsigned int, 10);
	struct peak_strings *root;
	unsigned int *ret, i = 0;

	root = peak_string_init();
	assert(root);

	assert(0 == peak_string_add(root, 1, " ipsum ", 0, STRING_RAW));
	assert(0 == peak_string_add(root, 1, NULL, 1, STRING_RAW));
	assert(1 == peak_string_add(root, 1, " ipsum ", 7, STRING_RAW));
	assert(2 == peak_string_add(root, 2, ", sed", 5, STRING_RAW));
	assert(3 == peak_string_add(root, 3, ", sef ", 5, STRING_RAW));
	assert(4 == peak_string_add(root, 4, ".", 1, STRING_RAW));
	assert(5 == peak_string_add(root, 5, "diam ", 4, STRING_RAW));
	assert(6 == peak_string_add(root, 6, "dolor sit amet,", 15, STRING_RAW));

	peak_string_find(root, text, strlen(text), stash);

	assert(STASH_COUNT(stash) == lengthof(result));

	STASH_FOREACH(ret, stash) {
		assert(*ret == result[i++]);
	}

	peak_string_exit(root);
}

int
main(void)
{
	pout("peak string test suite... ");

	test_string_single();
	test_string_multiple();
	test_string_wildcard();
	test_string_fi();
	test_string_la();
	test_string_fila();

	pout("ok\n");

	return (0);
}
