/*
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

#define STREAM_PAGE(x, y)						\
    (void *)((x)->page_mem + ((y) * (x)->page_size))
#define STREAM_TAIL(x, y)						\
    (void *)((uint8_t *)(y) + (x)->page_size - sizeof((y)->magic2))
#define STREAM_CMP(x, y) 	((x)->index - (y)->index)
#define STREAM_MAGIC		0xEF05BA06CD07FA08ull
#define STREAM_ERROR		-1

#define STREAM_SETUP(x, y, z) do {					\
	le64enc(STREAM_TAIL(x, y), STREAM_MAGIC);			\
	(y)->magic = STREAM_MAGIC;					\
	(y)->index = (z);						\
} while (0)

#define STREAM_CHECK(x, y) do {						\
	if (unlikely((y)->magic != STREAM_MAGIC ||			\
	    le64dec(STREAM_TAIL(x, y)) != STREAM_MAGIC)) {	\
		panic("page got trashed\n");				\
	}								\
} while (0)

#define STREAM_FROM_USER(x)	(((struct _peak_stream *)((x) + 1)) - 1)
#define STREAM_TO_USER(x)	&(x)->data

struct _peak_stream {
	ssize_t page_count;
	ssize_t page_no;
	struct peak_stream data;
};

RB_HEAD(peak_stream_tree, peak_stream_page);

struct peak_stream_page {
	uint64_t magic;
	ssize_t index;
	RB_ENTRY(peak_stream_page) rb_stream;
	/* size placeholder, sits at the end of the actual page: */
	uint64_t magic2;
};

struct peak_streams {
	struct peak_stream_tree page_tree;
	prealloc_t references;
	size_t page_count;
	size_t page_size;
	uint8_t *page_mem;
};

RB_GENERATE_STATIC(peak_stream_tree, peak_stream_page, rb_stream,
    STREAM_CMP);

static inline ssize_t
peak_stream_probe(struct peak_stream_page *base, const ssize_t stop)
{
	struct peak_stream_page *next = base;
	ssize_t count = 0;

	/*
	 * Count the number of consecutive pages in the
	 * free list.  To make the implementation more
	 * useful, the caller is in charge of defining the
	 * the start and stop position inside the free list.
	 * With the returned count the caller can jump to
	 * the next likely candidate and try again.
	 */

	for (;;) {
		if (!next) {
			/* no more elements */
			break;
		}

		if (count == stop) {
			/* request limit reached */
			break;
		}

		if (next->index != base->index + count) {
			/* scattered pages detected */
			break;
		}

		next = RB_NEXT(peak_stream_tree, next);
		++count;
	}

	return (count);
}

static inline void
peak_stream_unhook(struct peak_streams *self, struct peak_stream_page *page,
    ssize_t page_count)
{
	struct peak_stream_page *next;
	ssize_t i;

	for (i = 0; i < page_count; ++i) {
		STREAM_CHECK(self, page);
		next = RB_NEXT(peak_stream_tree, page);
		RB_REMOVE(peak_stream_tree, &self->page_tree, page);
		page = next;
	}
}

static ssize_t
__peak_stream_reclaim(struct peak_streams *self,
    struct _peak_stream *stream, ssize_t page_count)
{
	struct peak_stream_page *page;
	struct peak_stream_page ref;

	/* calculate the index of the needed page(s) */
	ref.index = stream->page_no + stream->page_count;

	/* see if we can find the next consecutive page */
	page = RB_FIND(peak_stream_tree, &self->page_tree, &ref);
	if (page_count > peak_stream_probe(page, page_count)) {
		/*
		 * At least one of the needed pages is blocked.
		 * In this case return with an error.  The caller
		 * knows that a fresh allocation might still be
		 * viable.
		 */
		return (STREAM_ERROR);
	}

	peak_stream_unhook(self, page, page_count);

	return (stream->page_no);
}

static ssize_t
__peak_stream_claim(struct peak_streams *self, ssize_t page_count)
{
	struct peak_stream_page *page;
	ssize_t i, j;

	/* since the stream is new, pick any page */
	page = RB_MIN(peak_stream_tree, &self->page_tree);

	for (;;) {
		i = peak_stream_probe(page, page_count);
		if (page_count <= i) {
			/* found a matching spot */
			break;
		}

		/* move past the scattered pages */
		for (j = 0; j < i; ++j) {
			page = RB_NEXT(peak_stream_tree, page);
		}

		if (!page) {
			/* hit the end of the list! */
			return (STREAM_ERROR);
		}
	}

	peak_stream_unhook(self, page, page_count);

	return (page->index);
}

static inline struct _peak_stream *
peak_stream_get(struct peak_streams *self)
{
	struct _peak_stream *ret = prealloc_get(&self->references);
	if (unlikely(!ret)) {
		/*
		 * The reference pool is filled with page_count
		 * items, which is the worst case: each stream
		 * holds one page.  Thus, it is safe to assume
		 * we have to panic here, because references
		 * have been leaked somewhere!
		 */
		panic("no more stream references available\n");
	}

	memset(ret, 0, sizeof(*ret));

	return (ret);
}

static unsigned int
_peak_stream_claim(struct peak_streams *self, struct peak_stream **ref,
    size_t size)
{
	struct _peak_stream *stream = NULL;
	ssize_t page_count, page_no;

	if (unlikely(!size)) {
		/* that went really well... */
		return (1);
	}

	if (*ref) {
		/* unwrap stream if it is set */
		stream = STREAM_FROM_USER(*ref);
	}

	if (unlikely(!stream && RB_EMPTY(&self->page_tree))) {
		/* no pages left, there's no use */
		return (0);
	}

	if (!stream) {
		page_count = (ALLOC_ALIGN(size, self->page_size) /
		    self->page_size);

		/*
		 * 1. Case: Fresh Allocation
		 *
		 *     There's no stream yet.  Find
		 *     pages and deal them out.
		 */
		page_no = __peak_stream_claim(self, page_count);
	} else {
		page_count = (ALLOC_ALIGN(size + stream->data.len,
		    self->page_size) / self->page_size) -
		    stream->page_count;

		/*
		 * 2. Case: Trivial Reallocation
		 *
		 *     "Give" the caller space.  It is already
		 *     available on the last allocated page.
		 */
		if (!page_count) {
			stream->data.len += size;
			return (1);
		}

		/*
		 * 3. Case: Normal Reallocation
		 *
		 *     We need more pages starting from the
		 *     end of the current stream's memory.
		 */
		page_no = __peak_stream_reclaim(self, stream, page_count);
	}

	if (page_no == STREAM_ERROR) {
		return (0);
	}

	stream = stream ? : peak_stream_get(self);

	stream->data.buf = STREAM_PAGE(self, page_no);
	stream->page_count += page_count;
	stream->page_no = page_no;
	stream->data.len += size;

	*ref = STREAM_TO_USER(stream);

	return (1);
}

unsigned int
peak_stream_claim(struct peak_streams *self, struct peak_stream **ref,
    size_t size)
{
	struct peak_stream *retry = NULL;
	unsigned int ret;

	ret = _peak_stream_claim(self, ref, size);
	if (ret || !*ref) {
		/* direct success or fresh allocation failure */
		return (ret);
	}

	/*
	 * There is no fast approach to grow the buffer.
	 * But we may still grow the buffer by claiming
	 * a brand new stream...
	 */
	ret = _peak_stream_claim(self, &retry, (*ref)->len + size);
	if (ret) {
		/*
		 * ... and flipping to the new stream reference.
		 * Copies the data to the new location.  This also
		 * realigns the data to the start of the page,
		 * reducing the stream's overall space consumption.
		 */
		memcpy(retry->buf, (*ref)->buf, (*ref)->len);
		peak_stream_release(self, ref, (*ref)->len);

		*ref = retry;
	}

	return (ret);
}

static void
_peak_stream_release(struct peak_streams *self, ssize_t page_no)
{
	struct peak_stream_page *page = STREAM_PAGE(self, page_no);

	STREAM_SETUP(self, page, page_no);

	if (unlikely(RB_INSERT(peak_stream_tree,
	    &self->page_tree, page))) {
		panic("duplicated page found: %zd\n", page_no);
	}
}

static inline void
peak_stream_put(struct peak_streams *self, struct _peak_stream *stream)
{
	prealloc_put(&self->references, stream);
}

void
peak_stream_release(struct peak_streams *self, struct peak_stream **ref,
    size_t size)
{
	struct _peak_stream *stream;
	size_t size_adj;

	if (unlikely(!size || !*ref)) {
		/* nothing to do */
		return;
	}

	stream = STREAM_FROM_USER(*ref);

	/* take current buffer offset into account */
	size_adj = (size_t)stream->data.buf;
	size_adj -= (size_t)STREAM_PAGE(self, stream->page_no);

	while (stream->page_count && size_adj + size >= self->page_size) {
		_peak_stream_release(self, stream->page_no);

		/* update accounting */
		stream->data.buf =
		    (uint8_t *)stream->data.buf + self->page_size;
		stream->data.len -= self->page_size;
		--stream->page_count;
		++stream->page_no;

		/* decrememt loop count */
		size -= self->page_size;
	}

	/* mop up remaining size (if any) */
	stream->data.buf = (uint8_t *)stream->data.buf + size;
	stream->data.len -= size;

	if (!stream->data.len) {
		/* no data left: delete reference */
		peak_stream_put(self, stream);
		*ref = NULL;
	}
}

void
peak_stream_exit(struct peak_streams *self)
{
	if (self) {
		prealloc_exit(&self->references);
		free(self->page_mem);
		free(self);
	}
}

struct peak_streams *
peak_stream_init(size_t page_count, size_t page_size)
{
	struct peak_streams *self = NULL;
	unsigned int ret;
	size_t i;

	if (!page_count) {
		goto peak_stream_init_fail;
	}

	if (page_size & 0x7) {
		/* only allow 8 byte aligned values */
		goto peak_stream_init_fail;
	}

	if (page_size < sizeof(struct peak_stream_page)) {
		/*
		 * Free pages are embedded into a tree
		 * structure, so the initial page size
		 * must hold the tree structure.  D'oh!
		 */
		goto peak_stream_init_fail;
	}

	self = calloc(1, sizeof(*self));
	if (!self) {
		goto peak_stream_init_fail;
	}

	self->page_mem = malign(page_count, page_size);
	if (!self->page_mem) {
		goto peak_stream_init_fail;
	}

	ret = prealloc_init(&self->references, page_count,
	    sizeof(struct _peak_stream));
	if (!ret) {
		goto peak_stream_init_fail;
	}

	self->page_count = page_count;
	self->page_size = page_size;
	RB_INIT(&self->page_tree);

	for (i = 0; i < page_count; ++i) {
		_peak_stream_release(self, i);
	}

	return (self);

peak_stream_init_fail:

	peak_stream_exit(self);
	return (NULL);
}
