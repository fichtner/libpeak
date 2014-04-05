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

struct peak_strings {
	struct _peak_strings *next;
	unsigned int result[STRING_MAX];
	int character;
};

PAGE_DECLARE(_peak_strings, struct peak_strings, 5);

#define STRING_CMP(x, y)	((x)->character - (y)->character)
#define STRING_ALLOC(x, y)	malloc(y)
#define STRING_FREE(x, y)	free(y)
#define STRING_WILDCARD		257

static void
_peak_string_find(struct peak_strings *node, const char *buf,
    const size_t len, const unsigned int start, stash_t stash)
{
	STASH_REBUILD(temp, unsigned int, stash);
	struct peak_strings stackptr(ref);
	struct peak_strings *found;

	if (node->result[STRING_RAW]) {
		/*
		 * There is no need for duplicated matches, but
		 * we'll do it anyway, e.g. "test test" produces
		 * two results of the same value.  It might fill
		 * up the stash, but then again maybe we can't be
		 * overly concerned.
		 */
		STASH_PUSH(&node->result[STRING_RAW], temp);
	}
	if (start && node->result[STRING_FI]) {
		STASH_PUSH(&node->result[STRING_FI], temp);
	}
	if (!len && node->result[STRING_LA]) {
		STASH_PUSH(&node->result[STRING_LA], temp);
	}
	if (!len && start && node->result[STRING_FILA]) {
		STASH_PUSH(&node->result[STRING_FILA], temp);
	}

	if (!len) {
		return;
	}

	/*
	 * search for literal character inside input buffer
	 */
	ref->character = (unsigned char)*buf;

	found = PAGE_FIND(node->next, ref, STRING_CMP);
	if (found) {
		_peak_string_find(found, buf + 1, len - 1, start, stash);
	}

	/*
	 * search for wildcard character by ignoring the input
	 */
	ref->character = STRING_WILDCARD;

	found = PAGE_FIND(node->next, ref, STRING_CMP);
	if (found) {
		_peak_string_find(found, buf + 1, len - 1, start, stash);
	}

}

void
peak_string_find(struct peak_strings *root, const char *buf,
    const size_t len, stash_t stash)
{
	size_t i;

	for (i = 0; i < len; ++i) {
		/*
		 * The recursive call produces spurious matches in
		 * case of e.g. searching for "test" in "testest".
		 * Not sure what to do about that...
		 */
		_peak_string_find(root, buf + i, len - i, !i, stash);
	}
}

int
peak_string_add(struct peak_strings *root, const unsigned int result,
    const char *buf, const size_t len, const unsigned int method)
{
	struct peak_strings stackptr(ref);
	struct peak_strings *node = root;
	unsigned int i;

	if (!buf || !len) {
		/*
		 * Don't allow empty matches.  This may sound
		 * paranoid, but, trust me, they will sneak in.
		 */
		return (0);
	}

	if (method >= STRING_MAX) {
		return (0);
	}

	bzero(ref, sizeof(*ref));

	for (i = 0; i < len; ++i) {
		if (buf[i] == '?') {
			ref->character = STRING_WILDCARD;
		} else {
			if (buf[i] == '\\' && i + 1 < len &&
			    (buf[i + 1] == '?' || buf[i + 1] == '\\')) {
				++i;
			}
			ref->character = (unsigned char)buf[i];
		}

		/*
		 * Reuse the available nodes if possible.
		 */
		node = (PAGE_INSERT(node->next, ref,
		    STRING_CMP, STRING_ALLOC, NULL));
		if (!node) {
			return (0);
		}
	}

	if (!node->result[method]) {
		/*
		 * Currently, we only have matching patterns,
		 * but sooner or later we want search options
		 * like anchor left/right or word boundary
		 * checks.  In that case the result needs to be
		 * a linked list with all the search options so
		 * that the "match" can be verified.
		 */
		node->result[method] = result;
	}

	return (node->result[method]);
}

static void
_peak_string_exit(struct peak_strings *node)
{
	struct _peak_strings *page;
	struct peak_strings *elem;

	if (!node) {
		return;
	}

	PAGE_FOREACH(elem, page, node->next) {
		_peak_string_exit(elem);
	}

	PAGE_COLLAPSE(node->next, STRING_FREE, NULL);
}

void
peak_string_exit(struct peak_strings *root)
{
	_peak_string_exit(root);
	free(root);
}

struct peak_strings *
peak_string_init(void)
{
	return (zalloc(sizeof(struct peak_strings)));
}
