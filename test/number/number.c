/*
 * Copyright (c) 2014 Masoud Chelongar <masoud@packetwerk.com>
 * Copyright (c) 2014 Franco Fichtner <franco@packetwerk.com>
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

static void
test_number_string(void)
{
	STASH_DECLARE(stash, unsigned int, 10);
	struct peak_numbers *root;
	unsigned int *ret;

	peak_number_exit(peak_number_init());

	root = peak_number_init();
	assert(root);

	/* finds nothing */
	peak_number_find(root, "30", 2, stash);
	assert(STASH_EMPTY(stash));

	/* duplicate the number */
	assert(1 == peak_number_add(root, 1, "404", 2, NUMBER_LT));
	assert(1 == peak_number_add(root, 2, "40", 2, NUMBER_LT));
	assert(3 == peak_number_add(root, 3, "44", 2, NUMBER_LT));
	assert(4 == peak_number_add(root, 4, "505", 3, 0));
	assert(5 == peak_number_add(root, 5, "505", 3, NUMBER_NE));
	assert(6 == peak_number_add(root, 6, "505", 3, NUMBER_GT));
	assert(7 == peak_number_add(root, 7, "505", 3, NUMBER_GE));
	assert(8 == peak_number_add(root, 8, "505", 3,
	    NUMBER_STRING | NUMBER_LE));

	/* finds nothing */
	peak_number_find(root, NULL, 0, stash);
	assert(STASH_EMPTY(stash));
	peak_number_find(root, "best", 4, stash);
	assert(STASH_EMPTY(stash));

	peak_number_find(root, "30 432 40 987 404", 17, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 3);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 5);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 8);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_number_find(root, "506", 3, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 5);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 6);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 7);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_number_find(root, "  505", 5, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 4);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 7);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 8);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_number_find(root, "505", 2, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 5);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 8);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_number_exit(NULL);
	peak_number_exit(root);
}

static void
test_number_length(void)
{
	STASH_DECLARE(stash, unsigned int, 10);
	struct peak_numbers *root;
	unsigned int *ret;

	root = peak_number_init();
	assert(root);

	assert(1 == peak_number_add(root, 1, "44", 2,
	    NUMBER_LENGTH | NUMBER_LE));

	peak_number_find(root, "", 30, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_number_find(root, "", 44, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_number_find(root, "", 45, stash);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_number_exit(root);
}

int
main(void)
{
	pout("peak number test suite... ");

	test_number_string();
	test_number_length();

	pout("ok\n");

	return (0);
}
