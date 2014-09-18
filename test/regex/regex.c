/*
 * Copyright (c) 2013 Masoud Chelongar <masoud@packetwerk.com>
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
 * Copyright (c) 2014 Tobias Boertitz <tobias@packetwerk.com>
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
test_regex(void)
{
	STASH_DECLARE(stash, unsigned int, 10);
	struct peak_regexes *root;
	unsigned int *ret;
	char *str;

	peak_regex_exit(peak_regex_init());

	root = peak_regex_init();
	assert(root);

	/* finds nothing */
	peak_regex_find(root, "get", 3, stash);
	assert(STASH_EMPTY(stash));

	/* duplicate regular expression */
	str = strdup("g[eo]t");
	assert(1 == peak_regex_add(root, 1, str, 6, 0));
	str[0] = '\0';
	free(str);	/* XXX use-after-free doesn't get detected */
	assert(1 == peak_regex_add(root, 2, "g[eo]t", 6, 0));
	assert(3 == peak_regex_add(root, 3, "g.t", 3, 0));
	assert(4 == peak_regex_add(root, 4, "OtG", 3, REGEX_NOCASE));
	assert(4 == peak_regex_add(root, 5, "OtG", 3, REGEX_NOCASE));
	assert(6 == peak_regex_add(root, 6, "OtG", 3, 0));

	/* finds nothing */
	peak_regex_find(root, "NULL", 0, stash);
	assert(STASH_EMPTY(stash));
	peak_regex_find(root, "gxx", 3, stash);
	assert(STASH_EMPTY(stash));

	/* multiple matches */
	peak_regex_find(root, "got get gotgit geo", 19, stash);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 1);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 3);
	assert((ret = STASH_POP(stash)));
	assert(*ret == 4);
	assert(STASH_EMPTY(stash));
	STASH_CLEAR(stash);

	peak_regex_exit(NULL);
	peak_regex_exit(root);
}

int
main(void)
{
	pout("peak regex test suite... ");

	test_regex();

	pout("ok\n");

	return (0);
}
