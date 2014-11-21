/*
 * Copyright (c) 2012-2014 Franco Fichtner <franco@packetwerk.com>
 * Copyright (c) 2014 Thomas Siegmund <thomas@packetwerk.com>
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

struct peak_jar_head {
	TAILQ_ENTRY(peak_jar_head) entry;
	unsigned int serial;
	unsigned int count;
	size_t write;
};

struct peak_jar_data {
	TAILQ_ENTRY(peak_jar_data) entry;
	unsigned int prev_serial;
	unsigned int serial;
	size_t len;
	unsigned char buf[];
};

#define JAR_HEAD_COUNT	1000

static inline void *
peak_jar_write(struct peak_jars *self, const size_t head_room)
{
	const size_t write = self->write;
	const size_t read = self->read;
	const size_t size = self->size;
	size_t new_write = write + head_room;
	void *ret = self->buffer + write;
	size_t free = read - write;

	/* in case write doesn't chase read pointer yet */
	if (write >= read) {
		free = size - write + read;
	}

	/*
	 * handle unlikely wrap around case
	 * (head_room must be contiguous)
	 */
	if (unlikely(new_write >= size)) {
		new_write = head_room;
		ret = self->buffer;
		free = read;
	}

	/* check if buffer is full */
	if (head_room >= free) {
		return (NULL);
	}

	/* sync new write value */
	self->write = new_write;

	/* return write location */
	return (ret);
}

static inline void
peak_jar_read(struct peak_jars *self)
{
	struct peak_jar_head *head = TAILQ_FIRST(&self->heads);
	if (head) {
		/*
		 * free old data sets and invalidate
		 * their associated serial number
		 */
		TAILQ_REMOVE(&self->heads, head, entry);
		self->read = head->write;
		++self->first_serial;
	}
}

static void
peak_jar_repair(struct peak_jars *self, struct peak_jar *context)
{
	struct peak_jar_data *data;

	/* is there any data in the context? */
	data = TAILQ_FIRST(&context->datas);
	if (!data) {
		return;
	}

	/* is there any data in the buffer? */
	if (TAILQ_EMPTY(&self->heads)) {
		TAILQ_INIT(&context->datas);
		return;
	}

	/* is the last serial valid? */
	if (wrap32(context->last_serial - self->first_serial)) {
		TAILQ_INIT(&context->datas);
		return;
	}

	/* is the first serial valid? */
	if (wrap32(context->first_serial - self->first_serial)) {
		struct peak_jar_data *zap = data;

		TAILQ_INIT(&context->datas);

		while (TAILQ_NEXT(zap, entry) &&
		    !wrap32(zap->prev_serial - self->first_serial)) {
			zap = TAILQ_NEXT(zap, entry);
			TAILQ_INSERT_TAIL(&context->datas, zap, entry);
		}

		context->first_serial = zap->serial;
	}
}

void
peak_jar_pack(struct peak_jars *self, struct peak_jar *context,
    const void *buf, const size_t len)
{
	struct peak_jar_head *head;
	struct peak_jar_data *data;

peak_jar_pack_again:

	/* do we need a new serial header? */
	head = TAILQ_LAST(&self->heads, peak_jar_header);
	if (unlikely(!head || head->count > JAR_HEAD_COUNT)) {
		head = peak_jar_write(self, sizeof(*head));
		if (!head) {
			peak_jar_read(self);
			goto peak_jar_pack_again;
		}

		/* initialise new serial header */
		TAILQ_INSERT_TAIL(&self->heads, head, entry);
		head->serial = self->last_serial++;
		head->write = self->write;
		head->count = 0;
	}

	/* allocate size for data header and data */
	data = peak_jar_write(self, sizeof(*data) +
	    ALLOC_ALIGN(len, sizeof(unsigned char *)));
	if (!data) {
		peak_jar_read(self);
		goto peak_jar_pack_again;
	}

	/* amend all invalid references in context */
	peak_jar_repair(self, context);

	/* initialise data header and copy data */
	memset(data, 0, sizeof(*data));
	memcpy(data->buf, buf, len);
	data->serial = head->serial;
	data->len = len;

	/* put into context (literally)  */
	if (TAILQ_EMPTY(&context->datas)) {
		context->first_serial = data->serial;
	}
	TAILQ_INSERT_HEAD(&context->datas, data, entry);
	context->last_serial = data->serial;

	/* almost done: sync prev_serial for peek */
	if (TAILQ_NEXT(data, entry)) {
		data->prev_serial =
		    TAILQ_NEXT(data, entry)->serial;
	}

	/* sync serial header */
	head->write = self->write;
	head->count += 1;
}

unsigned int
peak_jar_fifo(struct peak_jars *self, struct peak_jar *context,
    peak_jar_fun callback, void *userdata)
{
	struct peak_jar_data *data, *temp;

	/* amend all invalid references in the context */
	peak_jar_repair(self, context);

	/* invoke callback for all data in the context */
	TAILQ_FOREACH_REVERSE_SAFE(data, &context->datas,
	    peak_jar_user, entry, temp) {
		switch (callback(userdata, data->buf, data->len)) {
		case JAR_RETURN:
			goto peak_jar_fifo_out;
		case JAR_DROP:
			TAILQ_REMOVE(&context->datas, data, entry);
			/* FALLTHROUGH */
		case JAR_KEEP:
		default:
			break;
		}
	}

peak_jar_fifo_out:

	return (!TAILQ_EMPTY(&context->datas));
}

unsigned int
peak_jar_lifo(struct peak_jars *self, struct peak_jar *context,
    peak_jar_fun callback, void *userdata)
{
	struct peak_jar_data *data, *temp;

	/* amend all invalid references in the context */
	peak_jar_repair(self, context);

	/* invoke callback for all data in the context */
	TAILQ_FOREACH_SAFE(data, &context->datas, entry, temp) {
		switch (callback(userdata, data->buf, data->len)) {
		case JAR_RETURN:
			goto peak_jar_lifo_out;
		case JAR_DROP:
			TAILQ_REMOVE(&context->datas, data, entry);
			/* FALLTHROUGH */
		case JAR_KEEP:
		default:
			break;
		}
	}

peak_jar_lifo_out:

	return (!TAILQ_EMPTY(&context->datas));
}

unsigned int
peak_jar_init(struct peak_jars *self, size_t size)
{
	memset(self, 0, sizeof(*self));

	if (size & 0x7 || size < 128) {
		return (0);
	}

	self->buffer = malign(1, size);
	if (!self->buffer) {
		return (0);
	}

	TAILQ_INIT(&self->heads);
	self->size = size;
	self->write = 0;
	self->read = 0;

	return (1);
}

void
peak_jar_exit(struct peak_jars *self)
{
	free(self->buffer);
}
