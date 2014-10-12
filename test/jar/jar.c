/*
 * Copyright (c) 2012-2014 Franco Fichtner <franco@packetwerk.com>
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

static const char *stuff[] = {
	"what's", "all", "this", "then?",
};
static unsigned int count;

static unsigned int
test_callback(void *userdata, void *buf, size_t len)
{
	unsigned int *action = userdata;

	assert(strsize(buf) == len);
	assert(!memcmp(stuff[count], buf, len));
	++count;

	return (*action);
}

#define lifo(x, y, z)	peak_jar_lifo(x, y, test_callback, z)
#define fifo(x, y, z)	peak_jar_fifo(x, y, test_callback, z)
#define pack(x, y, z)	peak_jar_pack(x, y, z, strsize(z))

static void
test_jar(void)
{
	struct peak_jars bucket;
	struct peak_jar context;
	unsigned int action, i;

	JAR_INIT(&context);

	assert(!peak_jar_init(&bucket, 0));
	assert(!peak_jar_init(&bucket, 127));
	assert(!peak_jar_init(&bucket, 129));

	assert(peak_jar_init(&bucket, 200));
	peak_jar_exit(&bucket);
	assert(peak_jar_init(&bucket, 200));

	for (i = 0; i < lengthof(stuff); ++i) {
		pack(&bucket, &context, stuff[i]);
	}

	action = JAR_RETURN;
	count = 0;
	assert(fifo(&bucket, &context, &action));
	assert(count == 1);

	/* a little tricky: test last value only */
	action = JAR_RETURN;
	count = lengthof(stuff) - 1;
	assert(lifo(&bucket, &context, &action));
	assert(count == lengthof(stuff));

	action = JAR_KEEP;
	count = 0;
	assert(fifo(&bucket, &context, &action));
	assert(count == lengthof(stuff));

	action = JAR_DROP;
	count = 0;
	assert(!fifo(&bucket, &context, &action));
	assert(count == lengthof(stuff));

	action = JAR_KEEP;
	count = 0;
	assert(!fifo(&bucket, &context, &action));
	assert(!count);
	assert(!lifo(&bucket, &context, &action));
	assert(!count);

	pack(&bucket, &context, stuff[1]);
	action = JAR_KEEP;
	count = 1;
	assert(fifo(&bucket, &context, &action));
	assert(count == 2);

	peak_jar_exit(&bucket);
}

int
main(void)
{
	pout("peak jar test suite... ");

	test_jar();

	pout("ok\n");

	return (0);
}
