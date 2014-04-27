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
	struct peak_timeval tv;
	int64_t mono_usec_off;
	int64_t mono_msec_off;
	int64_t mono_sec_off;
	int64_t mono_usec;
	int64_t mono_msec;
	int64_t mono_sec;
	struct tm local;
	struct tm gmt;
	time_t time;
} timeslice_t;

#define _TIMESLICE_RECALIBRATE(now, prev, off) do {			\
	if (unlikely((now) - (off) < (prev))) {				\
		(off) = (now) - (prev);					\
	}								\
	(prev) = (now) - (off);						\
} while (0)

#define TIMESLICE_ADVANCE(clock, timeval) do {				\
	(clock)->tv = *(timeval);					\
	if (unlikely((timeval)->tv_sec != (clock)->time)) {		\
		(clock)->time = (timeval)->tv_sec;			\
		localtime_r(&(clock)->time, &(clock)->local);		\
		gmtime_r(&(clock)->time, &(clock)->gmt);		\
	}								\
	_TIMESLICE_RECALIBRATE((timeval)->tv_sec, (clock)->mono_sec,	\
	    (clock)->mono_sec_off);					\
	_TIMESLICE_RECALIBRATE((timeval)->tv_sec * 1000ll +		\
	    (timeval)->tv_usec / 1000ll, (clock)->mono_msec,		\
	    (clock)->mono_msec_off);					\
	_TIMESLICE_RECALIBRATE((timeval)->tv_sec * 1000ll * 1000ll +	\
	    (timeval)->tv_usec, (clock)->mono_usec,			\
	    (clock)->mono_usec_off);					\
} while (0)

#define TIMESLICE_CALIBRATE(clock, timeval) do {			\
	(clock)->mono_sec_off = (timeval)->tv_sec;			\
	(clock)->mono_msec_off = (timeval)->tv_sec * 1000ll +		\
	    (timeval)->tv_usec / 1000ll;				\
	(clock)->mono_usec_off = (timeval)->tv_sec * 1000ll * 1000ll +	\
	    (timeval)->tv_usec;						\
} while (0)

#define TIMESLICE_INIT(clock) do {					\
	bzero(clock, sizeof(timeslice_t));				\
} while (0)

#endif /* !PEAK_TIMESLICE_H */
