/*
 * Copyright (c) 2012-2013 Franco Fichtner <franco@packetwerk.com>
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
	uint64_t normal;
	uint64_t msec;
	uint64_t sec;
	/* hrm, Ubuntu doesn't build with the name "unix"
	 * and complains in a very cryptic way... */
	time_t epoch;
	struct tm local;
	struct tm gmt;
} timeslice_t;

#define TIMESLICE_ADVANCE(clock, ts_unix, ts_ms) do {			\
	(clock)->msec = (ts_ms) - (clock)->normal + 1000;		\
	(clock)->sec = (clock)->msec / 1000;				\
	if (unlikely((ts_unix) != (clock)->epoch)) {			\
		(clock)->epoch = (ts_unix);				\
		localtime_r(&(clock)->epoch, &(clock)->local);		\
		gmtime_r(&(clock)->epoch, &(clock)->gmt);		\
	}								\
} while (0)

#define TIMESLICE_NORMALISE(clock, ts_ms) do {				\
	(clock)->normal = (clock)->normal ? : (ts_ms);			\
} while (0)

#define TIMESLICE_INIT(clock) do {					\
	bzero(clock, sizeof(timeslice_t));				\
} while (0)

#endif /* !PEAK_TIMESLICE_H */
