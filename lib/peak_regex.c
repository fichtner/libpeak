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
#include <pcre.h>

struct peak_regexes {
	struct peak_regex *head;
};

struct peak_regex {
	struct peak_regex *next;
	unsigned int result;
	unsigned int flags;
	pcre_extra *pextra;
	char *pattern;
	pcre *regex;
};

void
peak_regex_find(struct peak_regexes *root, const char *buf,
    const size_t len, stash_t stash)
{
	struct peak_regex *node = root->head;
	STASH_REBUILD(temp, int, stash);
	int matches;

	if (!len) {
		return;
	}

	while (node) {
		/*
		 * Stop matching after the first regex hit.  Then
		 * move on to the next expression.  We need to go
		 * through the whole list each time, due to the
		 * fact that we cannot optimise regex matching on
		 * this meta level.
		 */

		matches = pcre_exec(node->regex, node->pextra, buf, len,
		    0, 0, NULL, 0);
		if (matches >= 0) {
			STASH_PUSH(&node->result, temp);
		}

		node = node->next;
	}
}

int
peak_regex_add(struct peak_regexes *root, const unsigned int result,
    const char *_buf, const size_t len, const unsigned int flags)
{
	unsigned int pcre_flags = 0;
	struct peak_regex *node;
	struct peak_regex *temp;
	char *buf;
	const char *error;
	int erroffset;

	if (!_buf || !len) {
		return (0);
	}

	if (flags & REGEX_NOCASE) {
		pcre_flags |= PCRE_CASELESS;
	}

	/*
	 * Put the string on the stack in order to get
	 * proper string termination going. This is
	 * because strings may not be null-terminated
	 * and pcre_compile(3) can't cope with that.
	 */
	buf = alloca(len + 1);
	strlcpy(buf, _buf, len + 1);

	temp = root->head;
	while (temp) {
		if (!strcmp(temp->pattern, buf) &&
		    pcre_flags == temp->flags) {
			return (temp->result);
		}

		temp = temp->next;
	}

	node = malloc(sizeof(*node));
	if (!node) {
		return (0);
	}

	node->regex = pcre_compile(buf, pcre_flags, &error, &erroffset, NULL);
	if (!node->regex) {
		/*
		 * Don't throw elaborate errors here.  The
		 * caller should have checked all regexes
		 * via peak_regex_parse() for validity.
		 */
		free(node);
		return (0);
	}

	/* try to speed up the matching */
	node->pextra = pcre_study(node->regex, 0, &error);
	if (!node->pextra && error) {
		/* no elaborate errors - see above */
		pcre_free(node->regex);
		free(node);
		return (0);
	}

	node->pattern = strdup(buf);
	if (!node->pattern) {
		pcre_free(node->regex);
		pcre_free(node->pextra);
		free(node);
		return (0);
	}

	/*
	 * Create a simple linked list of all added
	 * regexes to go through them one by one in
	 * peak_regex_find().
	 */

	node->flags = pcre_flags;
	node->next = root->head;
	node->result = result;

	root->head = node;

	return (node->result);
}

const char *
peak_regex_parse(const char *buf, const size_t len, const unsigned int flags)
{
	unsigned int pcre_flags = 0;
	pcre_extra *pextra = NULL;
	const char *ret = NULL;
	pcre *regex = NULL;
	const char *error;
	int erroffset;

	if (!buf || !len) {
		return ("empty regular expression");
	}

	if (flags & REGEX_NOCASE) {
		pcre_flags |= PCRE_CASELESS;
	}

	regex = pcre_compile(buf, pcre_flags, &error, &erroffset, NULL);
	if (!regex) {
		ret = error;
	} else {
		pextra = pcre_study(regex, 0, &error);
		if (!pextra && error) {
			ret = error;
		}
	}

	pcre_free(regex);
	pcre_free(pextra);

	return (ret);
}

void
peak_regex_exit(struct peak_regexes *root)
{
	struct peak_regex *node;
	struct peak_regex *temp;

	if (!root) {
		return;
	}

	node = root->head;
	while (node) {
		temp = node;
		node = node->next;
		pcre_free(temp->regex);
		pcre_free(temp->pextra);
		free(temp->pattern);
		free(temp);
	}

	free(root);
}

struct peak_regexes *
peak_regex_init(void)
{
	return (calloc(1, sizeof(struct peak_regexes)));
}
