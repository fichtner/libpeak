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

#ifndef PEAK_TOKEN_H
#define PEAK_TOKEN_H

struct peak_token {
	int64_t have;		/* current number of tokens */
	int64_t max;		/* maximum number of tokens per second */
	int64_t ts_ms;		/* last updated millisecond timestamp */
	spinlock_t lock;	/* lock for concurrent access */
};

static inline unsigned int
_peak_token_credit(struct peak_token *self, const int64_t want,
    const int64_t ts_ms)
{
	const int64_t credit = ts_ms - self->ts_ms;

	if (!self->max) {
		/* and I'm like, yeah, whatever */
		return (1);
	}

	if (credit > 0) {
		self->ts_ms = ts_ms;
		self->have += (credit * self->max) / 1000ll;
		/*
		 * Floor current in case it's bigger than maximum
		 * so that we do not offer free magic bandwidth.
		 */
		if (self->have > self->max) {
			self->have = self->max;
		}
	}

	/*
	 * Do not punish traffic that's ultimately not
	 * sendable due to a couple of missing bytes.
	 * We are all about sharing and since we can go
	 * negative the next operation will reimburse the
	 * little burst we give below...
	 */
	if (want > 0 && self->have <= 0) {
		return (0);
	}

	/* in the case this is a reimburse, want was negative */
	self->have -= want;

	return (1);
}

static inline unsigned int
peak_token_credit(struct peak_token *self, const int64_t want,
    const int64_t ts_ms)
{
	unsigned int ret;

	spin_lock(&self->lock);
	ret = _peak_token_credit(self, want, ts_ms);
	spin_unlock(&self->lock);

	return (ret);
}

static inline void
peak_token_exit(struct peak_token *self)
{
	spin_exit(&self->lock);
}

static inline void
_peak_token_init(struct peak_token *self, const int64_t max)
{
	self->max = self->have = max;
	self->ts_ms = 0;
}

static inline void
peak_token_init(struct peak_token *self, const int64_t max)
{
	spin_init(&self->lock);
	_peak_token_init(self, max);
}

#endif /* !PEAK_TOKEN_H */
