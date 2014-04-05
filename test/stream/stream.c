/*
 * Copyright (c) 2013 Franco Fichtner <franco@packetwerk.com>
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

#define PAGE_SIZE	4096	/* average size */
#define DATA_SIZE	56	/* minimum size */

static void
test_stream_basics(void)
{
	struct peak_stream *ref1 = NULL;
	struct peak_stream *ref2 = NULL;
	struct peak_stream *ref3 = NULL;
	struct peak_streams *pool;

	/* empty pools are useless */
	assert(!peak_stream_init(0, PAGE_SIZE));

	/* page size minimum underrun */
	assert(!peak_stream_init(1, 8));

	/* page size not aligned to 8 bytes */
	assert(!peak_stream_init(1, PAGE_SIZE + 3));

	/* valid pool init */
	pool = peak_stream_init(2, PAGE_SIZE);
	assert(pool);

	/* empty claim successfully does nothing */
	assert(peak_stream_claim(pool, &ref1, 0));
	assert(!ref1);

	/* allocate two pages at once */
	assert(peak_stream_claim(pool, &ref1, PAGE_SIZE * 2));
	assert(ref1);

	/* no more pages available :( */
	assert(!peak_stream_claim(pool, &ref2, PAGE_SIZE));
	assert(!ref2);

	/* free both pages */
	peak_stream_release(pool, &ref1, PAGE_SIZE * 2);
	assert(!ref1);

	/* claim all pages, fail on empty */
	assert(peak_stream_claim(pool, &ref1, PAGE_SIZE));
	assert(ref1);
	assert(ref1->len == PAGE_SIZE);
	assert(ref1->buf);
	assert(peak_stream_claim(pool, &ref2, PAGE_SIZE));
	assert(ref2);
	assert(ref2->len == PAGE_SIZE);
	assert(ref2->buf);
	assert(!peak_stream_claim(pool, &ref3, PAGE_SIZE));
	assert(!ref3);

	/* free all pages */
	peak_stream_release(pool, &ref3, PAGE_SIZE);
	peak_stream_release(pool, &ref2, PAGE_SIZE);
	peak_stream_release(pool, &ref1, PAGE_SIZE);
	assert(!ref1 && !ref2 && !ref3);

	/* don't panic */
	peak_stream_exit(NULL);

	/* proper exit */
	peak_stream_exit(pool);
}

static void
test_stream_shrink(void)
{
	struct peak_stream *ref1 = NULL;
	struct peak_stream *ref2 = NULL;
	struct peak_stream *ref3 = NULL;
	struct peak_streams *pool;

	pool = peak_stream_init(6, PAGE_SIZE);
	assert(pool);

	/* grab too many pages */
	assert(!peak_stream_claim(pool, &ref1, 7 * PAGE_SIZE));
	assert(!ref1);

	/* grab all pages */
	assert(peak_stream_claim(pool, &ref1, 6 * PAGE_SIZE));
	assert(ref1);

	/* release 3 pages */
	peak_stream_release(pool, &ref1, 3 * PAGE_SIZE);
	assert(ref1);

	/* grab released pages again */
	assert(peak_stream_claim(pool, &ref2, 2 * PAGE_SIZE));
	assert(ref2);

	/* can't grab two more pages */
	assert(!peak_stream_claim(pool, &ref3, 2 * PAGE_SIZE));
	assert(!ref3);

	/* release remaining 3 pages */
	peak_stream_release(pool, &ref1, PAGE_SIZE);
	assert(ref1);
	peak_stream_release(pool, &ref1, PAGE_SIZE);
	assert(ref1);
	peak_stream_release(pool, &ref1, PAGE_SIZE);
	assert(!ref1);

	/* free last two pages */
	peak_stream_release(pool, &ref2, 2 * PAGE_SIZE);
	assert(!ref2);

	peak_stream_exit(pool);
}

static void
test_stream_grow(void)
{
	struct peak_stream *ref1 = NULL;
	struct peak_stream *ref2 = NULL;
	struct peak_stream *ref3 = NULL;
	struct peak_streams *pool;

	pool = peak_stream_init(6, PAGE_SIZE);
	assert(pool);

	/* acquire a single page */
	assert(peak_stream_claim(pool, &ref1, PAGE_SIZE));
	assert(ref1);

	/* get another page on top of the first one */
	assert(peak_stream_claim(pool, &ref1, PAGE_SIZE));
	assert(ref1);

	/* block next page by giving it to ref2 */
	assert(peak_stream_claim(pool, &ref2, PAGE_SIZE));
	assert(ref2);

	/* grow and relocate due to missing page */
	assert(peak_stream_claim(pool, &ref1, PAGE_SIZE));
	assert(ref1);

	/* get the indirectly released pages */
	assert(peak_stream_claim(pool, &ref3, 2 * PAGE_SIZE));
	assert(ref3);

	/* remove all memory in one operation */
	peak_stream_release(pool, &ref3, 2 * PAGE_SIZE);
	peak_stream_release(pool, &ref2, PAGE_SIZE);
	peak_stream_release(pool, &ref1, 3 * PAGE_SIZE);

	peak_stream_exit(pool);
}

static void
test_stream_data(void)
{
	struct peak_stream *ref1 = NULL;
	struct peak_stream *ref2 = NULL;
	struct peak_streams *pool;
	uint64_t *ptr, v = 0;
	unsigned int i;

	pool = peak_stream_init(4, DATA_SIZE);
	assert(pool);

	/* get a data slice */
	assert(peak_stream_claim(pool, &ref1, DATA_SIZE));
	assert(ref1);

	/* put in a few values to test consistency */
	for (i = 0, ptr = ref1->buf; i < ref1->len / sizeof(v); ++i) {
		ptr[i] = i;
	}

	/* block next page to force migration of ref1 */
	assert(peak_stream_claim(pool, &ref2, DATA_SIZE));
	assert(ref2);

	/* read data, it should be perfectly safe */
	for (i = 0, ptr = ref1->buf; i < ref1->len / sizeof(v); ++i) {
		assert(ptr[i] == i);
	}

	/* relocate the first stream and check data afterwards */
	assert(peak_stream_claim(pool, &ref1, DATA_SIZE));
	assert(ref1);

	/* read first block of data, it should be perfectly safe */
	for (i = 0, ptr = ref1->buf; i < ref1->len / 2 / sizeof(v); ++i) {
		assert(ptr[i] == i);
	}

	/* put in second block of data */
	for (; i < ref1->len / sizeof(v); ++i) {
		ptr[i] = i;
	}

	/* release unneeded block */
	peak_stream_release(pool, &ref2, DATA_SIZE);
	assert(!ref2);

	/* release first used block, buffer pointer moves forward */
	peak_stream_release(pool, &ref1, DATA_SIZE);
	assert(ref1 && ref1->buf != ptr);

	/* second bit of data still in place? */
	for (i = 0, ptr = ref1->buf; i < ref1->len / sizeof(v); ++i) {
		assert(ptr[i] == i + (ref1->len / sizeof(v)));
	}

	/* release last block */
	peak_stream_release(pool, &ref1, DATA_SIZE);
	assert(!ref1);

	peak_stream_exit(pool);
}

static void
test_stream_byte_simple(void)
{
	struct peak_stream *ref = NULL;
	struct peak_streams *pool;

	pool = peak_stream_init(1, DATA_SIZE);
	assert(pool);

	/* only claim half the page size */
	assert(peak_stream_claim(pool, &ref, DATA_SIZE / 2));
	assert(ref && ref->buf && ref->len == DATA_SIZE / 2);

	/* claim the other half now */
	assert(peak_stream_claim(pool, &ref, DATA_SIZE / 2));
	assert(ref && ref->buf && ref->len == DATA_SIZE);

	/* release first half */
	peak_stream_release(pool, &ref, DATA_SIZE / 2);
	assert(ref && ref->buf && ref->len == DATA_SIZE / 2);

	/* release everything */
	peak_stream_release(pool, &ref, DATA_SIZE / 2);
	assert(!ref);

	peak_stream_exit(pool);
}

static const char *test_stream_strings[] = {
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
	"Curabitur sed turpis felis.",
	"Vestibulum malesuada ligula at commodo suscipit.",
	"Aliquam erat volutpat.",
	"Etiam at pulvinar metus, nec porttitor mi.",
	"Maecenas odio nunc, venenatis a fringilla vehicula, "
	"ultrices sed tellus.",
	"Cras id diam vitae lectus mollis tempor.",
	"Integer dignissim turpis vitae interdum mattis.",
	"Pellentesque habitant morbi tristique senectus et netus et "
	"malesuada fames ac turpis egestas.",
	"Integer et ligula massa.",
	"Nunc non tortor tempus, rhoncus arcu sed, volutpat sapien.",
	"Praesent pulvinar massa iaculis elit tincidunt faucibus.",
	"Etiam fringilla egestas dignissim.",
	"Etiam dignissim varius nisl, "
	"ut tincidunt dolor vestibulum accumsan.",
	"Integer nec urna sed turpis dapibus euismod.",
	"Nam vestibulum, tellus et pharetra mollis, "
	"mauris leo adipiscing quam, in viverra felis tellus ac libero.",
	"Suspendisse potenti.",
	"Maecenas leo mi, adipiscing sed eleifend nec, "
	"consequat vel felis.",
	"Proin ac posuere orci, vel congue turpis.",
	"Donec vestibulum orci a tristique tincidunt.",
	"Morbi eget sollicitudin urna, sit amet faucibus magna.",
	"Vestibulum et eleifend nunc.",
	"Praesent ut dolor volutpat, imperdiet nisi id, commodo urna.",
	"Fusce sit amet nisl quam.",
	"Donec quis massa eu turpis porttitor sollicitudin.",
	"Pellentesque feugiat elit in urna lacinia luctus.",
};

static void
_test_stream_byte_complex_in(struct peak_streams *pool,
    struct peak_stream **ref, const unsigned int pos)
{
	const unsigned int len = strsize(test_stream_strings[pos]);

	assert(peak_stream_claim(pool, ref, len));
	memcpy((uint8_t *)(*ref)->buf + (*ref)->len - len,
	    test_stream_strings[pos], len);
}

static void
_test_stream_byte_complex_out(struct peak_streams *pool,
    struct peak_stream **ref, const unsigned int pos)
{
	const unsigned int len = strsize(test_stream_strings[pos]);

	assert(!strcmp((*ref)->buf, test_stream_strings[pos]));
	peak_stream_release(pool, ref, len);
}

static void
test_stream_byte_complex(void)
{
	const unsigned int j = lengthof(test_stream_strings);
	struct peak_stream *forward = NULL;
	struct peak_stream *reverse = NULL;
	struct peak_streams *pool;
	unsigned int i;

	pool = peak_stream_init(600, DATA_SIZE);
	assert(pool);

	/*
	 * Put all strings into the streams one by one.
	 * Do this once in order and once in reverse to
	 * shuffle things around a bit:
	 */
	for (i = 0; i < j; ++i) {
		_test_stream_byte_complex_in(pool, &forward, i);
		_test_stream_byte_complex_in(pool, &reverse, j - i - 1);
	}

	/*
	 * Now, verify all strings and release them,
	 * again one by one.  Piece of cake, right?
	 */
	for (i = 0; i < j; ++i) {
		_test_stream_byte_complex_out(pool, &forward, i);
		_test_stream_byte_complex_out(pool, &reverse, j - i - 1);
	}

	peak_stream_exit(pool);
}

int
main(void)
{
	pout("peak stream test suite... ");

	test_stream_basics();
	test_stream_shrink();
	test_stream_grow();
	test_stream_data();
	test_stream_byte_simple();
	test_stream_byte_complex();

	pout("ok\n");

	return (0);
}
