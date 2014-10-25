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
#include <compat.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#define NUMBER_TYPE_MASK	0xFF00
#define NUMBER_OP_MASK		0x00FF

struct peak_numbers {
	struct peak_number *string;
	struct peak_number *length;
};

struct peak_number {
	struct peak_number *next;
	unsigned int operator;
	unsigned int result;
	long long number;
};

static inline unsigned int
peak_number_get(const char *buf, long long *ret)
{
	long long lval;
	char *ep;

	errno = 0;

	lval = strtoll(buf, &ep, 10);
	if (buf[0] == '\0' || buf == ep || errno == EINVAL) {
		return (1);
	}
	if (errno == ERANGE && (lval == LLONG_MAX || lval == LLONG_MIN)) {
		return (1);
	}

	*ret = lval;

	return (0);
}

static void
peak_number_match(struct peak_number *node, const long long number,
    stash_t stash)
{
	STASH_REBUILD(temp, int, stash);

	while (node) {
		unsigned int ret = 0;

		switch (node->operator) {
		case NUMBER_GT:
			ret = number > node->number;
			break;
		case NUMBER_LT:
			ret = number < node->number;
			break;
		case NUMBER_EQ:
			ret = number == node->number;
			break;
		case NUMBER_NE:
			ret = number != node->number;
			break;
		case NUMBER_LE:
			ret = number <= node->number;
			break;
		case NUMBER_GE:
			ret = number >= node->number;
			break;
		default:
			break;
		}

		if (ret) {
			STASH_PUSH(&node->result, temp);
		}

		node = node->next;
	}
}

void
peak_number_find(struct peak_numbers *self, const char *_buf,
    const size_t len, stash_t stash)
{
	long long number;
	char *buf;

	peak_number_match(self->length, len, stash);

	if (!len) {
		return;
	}

	buf = alloca(len + 1);
	strlcpy(buf, _buf, len + 1);

	if (peak_number_get(buf, &number)) {
		return;
	}

	peak_number_match(self->string, number, stash);
}

unsigned int
peak_number_add(struct peak_numbers *self, const unsigned int result,
    const char *_buf, const size_t len, const unsigned int flags)
{
	const unsigned int type = flags & NUMBER_TYPE_MASK;
	const unsigned int op = flags & NUMBER_OP_MASK;
	struct peak_number **root;
	struct peak_number *node;
	struct peak_number *temp;
	long long number;
	char *buf;

	if (!_buf || !len) {
		return (0);
	}

	if (op >= NUMBER_MAX) {
		return (0);
	}

	buf = alloca(len + 1);
	strlcpy(buf, _buf, len + 1);

	if (peak_number_get(buf, &number)) {
		return (0);
	}

	switch (type) {
	case NUMBER_LENGTH:
		root = &self->length;
		break;
	case NUMBER_STRING:
	default:
		root = &self->string;
		break;
	}

	temp = *root;
	while (temp) {
		if (temp->number == number && temp->operator == op) {
			return (temp->result);
		}

		temp = temp->next;
	}

	node = malloc(sizeof(*node));
	if (!node) {
		return (0);
	}

	/*
	 * Create a simple linked list of all added
	 * numbers to go through them one by one in
	 * peak_number_find().
	 */

	node->next = *root;
	node->result = result;
	node->number = number;
	node->operator = op;

	*root = node;

	return (node->result);
}

void
peak_number_exit(struct peak_numbers *self)
{
	struct peak_number *node;
	struct peak_number *temp;

	if (!self) {
		return;
	}

	node = self->string;
	while (node) {
		temp = node;
		node = node->next;
		free(temp);
	}

	node = self->length;
	while (node) {
		temp = node;
		node = node->next;
		free(temp);
	}

	free(self);
}

const char *
peak_number_parse(const char *_buf, const size_t len)
{
	const char *ret = NULL;
	long long number;
	char *buf;

	if (!_buf || !len) {
		ret = "need a number";
	}

	buf = alloca(len + 1);
	strlcpy(buf, _buf, len + 1);

	if (peak_number_get(buf, &number)) {
		ret = "number conversion failed";
	}

	return (ret);
}

struct peak_numbers *
peak_number_init(void)
{
	return (calloc(1, sizeof(struct peak_numbers)));
}
