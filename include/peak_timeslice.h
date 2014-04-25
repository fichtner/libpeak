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

#ifndef PEAK_TIMESLICE_H
#define PEAK_TIMESLICE_H

#include <time.h>

typedef struct {
	int64_t offset;
	int64_t msec;
	int64_t sec;
	struct timeval tv;
	struct tm local;
	struct tm gmt;
} timeslice_t;

#define TIMESLICE_ADVANCE(clock, ts_sec, ts_ms) do {			\
	(clock)->msec = (ts_ms) - (clock)->offset;			\
	(clock)->sec = (clock)->msec / 1000;				\
	if (unlikely((ts_sec) != (clock)->tv.tv_sec)) {			\
		(clock)->tv.tv_sec = (ts_sec);				\
		localtime_r(&(clock)->tv.tv_sec, &(clock)->local);	\
		gmtime_r(&(clock)->tv.tv_sec, &(clock)->gmt);		\
	}								\
} while (0)

#define TIMESLICE_NORMALISE(clock, ts_sec) do {				\
	(clock)->offset = (ts_sec);					\
	(clock)->offset *= 1000;					\
} while (0)

#define TIMESLICE_INIT(clock) do {					\
	bzero(clock, sizeof(timeslice_t));				\
} while (0)

#endif /* !PEAK_TIMESLICE_H */
