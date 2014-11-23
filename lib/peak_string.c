/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
 * Copyright (c) 2014 Masoud Chelongar <masoud@packetwerk.com>
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

enum {
	STRING_METHOD_FLOATING,
	STRING_METHOD_LEFT,
	STRING_METHOD_RIGHT,
	STRING_METHOD_EXACT,
	STRING_METHOD_MAX	/* last element */
};

static const int string_nocase[256] = {
	/* uninitialised fields are set to zero... */
	['a'] = 257, ['A'] = 257,
	['b'] = 258, ['B'] = 258,
	['c'] = 259, ['C'] = 259,
	['d'] = 260, ['D'] = 260,
	['e'] = 261, ['E'] = 261,
	['f'] = 262, ['F'] = 262,
	['g'] = 263, ['G'] = 263,
	['h'] = 264, ['H'] = 264,
	['i'] = 265, ['I'] = 265,
	['j'] = 266, ['J'] = 266,
	['k'] = 267, ['K'] = 267,
	['l'] = 268, ['L'] = 268,
	['m'] = 269, ['M'] = 269,
	['n'] = 270, ['N'] = 270,
	['o'] = 271, ['O'] = 271,
	['p'] = 272, ['P'] = 272,
	['q'] = 273, ['Q'] = 273,
	['r'] = 274, ['R'] = 274,
	['s'] = 275, ['S'] = 275,
	['t'] = 276, ['T'] = 276,
	['u'] = 277, ['U'] = 277,
	['v'] = 278, ['V'] = 278,
	['w'] = 279, ['W'] = 279,
	['x'] = 280, ['X'] = 280,
	['y'] = 281, ['Y'] = 281,
	['z'] = 282, ['Z'] = 282,
};

struct peak_strings {
	struct _peak_strings *next;
	unsigned int result[STRING_METHOD_MAX];
	int character;
};

PAGE_DECLARE(_peak_strings, struct peak_strings, 5);

#define STRING_CMP(x, y)	((x)->character - (y)->character)
#define STRING_ALLOC(x, y)	malloc(y)
#define STRING_FREE(x, y)	free(y)
#define STRING_WILDCARD		256

static unsigned int
peak_string_match(const struct peak_strings *node, const size_t len,
    const unsigned int start, stash_t stash)
{
	STASH_REBUILD(temp, unsigned int, stash);

	if (node->result[STRING_METHOD_FLOATING]) {
		/*
		 * There is no need for duplicated matches, but
		 * we'll do it anyway, e.g. "test test" produces
		 * two results of the same value.
		 */
		STASH_PUSH(&node->result[STRING_METHOD_FLOATING], temp);
	}

	if (node->result[STRING_METHOD_LEFT] && start) {
		STASH_PUSH(&node->result[STRING_METHOD_LEFT], temp);
	}

	if (node->result[STRING_METHOD_RIGHT] && !len) {
		STASH_PUSH(&node->result[STRING_METHOD_RIGHT], temp);
	}

	if (node->result[STRING_METHOD_EXACT] && start && !len) {
		STASH_PUSH(&node->result[STRING_METHOD_EXACT], temp);
	}

	return !len;
}

static void
_peak_string_find(struct peak_strings *node, const char *buf,
    const size_t len, const unsigned int start, stash_t stash)
{
	struct peak_strings stackptr(ref);
	struct peak_strings *found;

	/*
	 * search for literal character inside input buffer
	 */
	ref->character = (unsigned char)*buf;

	found = PAGE_FIND(node->next, ref, STRING_CMP);
	if (found) {
		if (peak_string_match(found, len - 1, start, stash)) {
			return;
		}
		_peak_string_find(found, buf + 1, len - 1, start, stash);
	}

	/*
	 * search for wildcard character by ignoring the input
	 */
	ref->character = STRING_WILDCARD;

	found = PAGE_FIND(node->next, ref, STRING_CMP);
	if (found) {
		if (peak_string_match(found, len - 1, start, stash)) {
			return;
		}
		_peak_string_find(found, buf + 1, len - 1, start, stash);
	}

	/*
	 * search for case insensitive character inside input buffer
	 */
	ref->character = string_nocase[(unsigned char)*buf];

	if (ref->character) {
		found = PAGE_FIND(node->next, ref, STRING_CMP);
		if (found) {
			if (peak_string_match(found, len - 1,
			    start, stash)) {
				return;
			}
			_peak_string_find(found, buf + 1, len - 1,
			    start, stash);
		}
	}
}

void
peak_string_find(struct peak_strings *root, const char *buf,
    const size_t len, stash_t stash)
{
	size_t i;

	if (peak_string_match(root, len, 1, stash)) {
		return;
	}

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
    const char *buf, const size_t len, const unsigned int flags)
{
	struct peak_strings stackptr(ref);
	struct peak_strings *node = root;
	unsigned int i, method;

	if (!buf) {
		return (0);
	}

	memset(ref, 0, sizeof(*ref));

	for (i = 0; i < len; ++i) {
		if (buf[i] == '?') {
			ref->character = STRING_WILDCARD;
		} else {
			if (buf[i] == '\\' && i + 1 < len &&
			    (buf[i + 1] == '?' || buf[i + 1] == '\\')) {
				++i;
			}
			ref->character = (unsigned char)buf[i];

			if (flags & STRING_NOCASE) {
				int nocase = string_nocase[ref->character];
				if (nocase) {
					ref->character = nocase;
				}
			}
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

	if ((flags & STRING_LEFT) && (flags & STRING_RIGHT)) {
		method = STRING_METHOD_EXACT;
	} else if (flags & STRING_LEFT) {
		method = STRING_METHOD_LEFT;
	} else if (flags & STRING_RIGHT) {
		method = STRING_METHOD_RIGHT;
	} else {
		method = STRING_METHOD_FLOATING;
	}

	if (!node->result[method]) {
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
	return (calloc(1, sizeof(struct peak_strings)));
}
