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

struct peak_timeval {
	/* it's bound to happen... */
	long long tv_sec;
	long tv_usec;
};

typedef struct {
	int64_t mono_off;
	int64_t mono_sec;
	int64_t mono_msec;
	int64_t mono_usec;
	struct peak_timeval tv;
	time_t time;
	struct tm local;
	struct tm gmt;
} timeslice_t;

#define TIMESLICE_ADVANCE(clock, ts_sec, ts_usec) do {			\
	(clock)->tv.tv_usec = (ts_usec);				\
	(clock)->tv.tv_sec = (ts_sec);					\
	(clock)->mono_usec = (ts_sec) * 1000ll * 1000ll + (ts_usec);	\
	(clock)->mono_msec = (ts_sec) * 1000ll + (ts_usec) / 1000ll;	\
	(clock)->mono_sec = (ts_sec);					\
	(clock)->mono_sec -= (clock)->mono_off;				\
	(clock)->mono_msec -= (clock)->mono_off * 1000ll;		\
	(clock)->mono_usec -= (clock)->mono_off * 1000ll * 1000ll;	\
	if (unlikely((ts_sec) != (clock)->time)) {			\
		(clock)->time = (ts_sec);				\
		localtime_r(&(clock)->time, &(clock)->local);		\
		gmtime_r(&(clock)->time, &(clock)->gmt);		\
	}								\
} while (0)

#define TIMESLICE_NORMALISE(clock, ts_sec) do {				\
	(clock)->mono_off = (ts_sec);					\
} while (0)

#define TIMESLICE_INIT(clock) do {					\
	bzero(clock, sizeof(timeslice_t));				\
} while (0)

#endif /* !PEAK_TIMESLICE_H */
