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

#ifndef PEAK_OUTPUT_H
#define PEAK_OUTPUT_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

extern int _peak_bug_priority;

static inline void
_peak_print(FILE *stream, const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vfprintf(stream, message, ap);
	va_end(ap);
}

/* keep syslog style layout so we can switch on demand later */
#define __peak_bug(x, y, args...) do {					\
	_peak_print(stderr, y, ##args);					\
} while (0)

/* additional wrapper with access to the format string */
#define _peak_bug(x, y, args...) do {					\
	static const char *_msg = y;					\
	if ((x) <= __sync_fetch_and_or(&_peak_bug_priority, 0)) {	\
		__peak_bug(x, _msg, ##args);				\
	}								\
} while (0)

/* keep syslog style layout so we can switch on demand later */
#define _peak_log(x, y, args...) do {					\
	static const char *_msg = y;					\
	_peak_print(stdout, _msg, ##args);				\
} while (0)

/* fast stdout/stderr access (handle with care) */
#define _peak_out(x, args...)	_peak_print(stdout, x, ##args)
#define _peak_err(x, args...)	_peak_print(stderr, x, ##args)

#define BACKTRACE_NORMAL	1
#define BACKTRACE_SIGNAL	3
#define BACKTRACE_MAX		128

#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__)
#include <execinfo.h>

static inline void
_peak_backtrace(const int skip)
{
	void *callstack[BACKTRACE_MAX];
	int frames;

	_peak_bug(LOG_EMERG, "====== stack trace begin ======\n");
	frames = backtrace(callstack, BACKTRACE_MAX) - skip;
	backtrace_symbols_fd(&callstack[skip], frames, 2);
	_peak_bug(LOG_EMERG, "======= stack trace end =======\n");
}
#else /* !__APPLE__ && !__linux__ && !__FreeBSD__ */
#define _peak_backtrace(x)
#endif /* __APPLE__ || __linux__ || __FreeBSD__ */

#undef BACKTRACE_MAX

/* shorter is always better */
#define pbacktrace	_peak_backtrace
#define pbug		_peak_bug
#define plog		_peak_log
#define perr		_peak_err
#define pout		_peak_out

/* fiddle around with macros to make panics show file/line info */
#define _STRING_HACK(x)	#x
#define STRING_HACK(x)	_STRING_HACK(x)
#define WHERE_HACK	__FILE__ ":" STRING_HACK(__LINE__) ": "

#define panic(msg, args...) do {					\
	_peak_bug(LOG_EMERG, WHERE_HACK msg, ##args);			\
	_peak_backtrace(BACKTRACE_NORMAL);				\
	abort();							\
} while (0)

#define alert(msg, args...) do {					\
	_peak_bug(LOG_ALERT, WHERE_HACK msg, ##args);			\
} while (0)

#define critical(msg, args...) do {					\
	_peak_bug(LOG_CRIT, WHERE_HACK msg, ##args);			\
} while (0)

#define error(msg, args...) do {					\
	_peak_bug(LOG_ERR, WHERE_HACK msg, ##args);			\
} while (0)

#define warning(msg, args...) do {					\
	_peak_bug(LOG_WARNING, WHERE_HACK msg, ##args);			\
} while (0)

#define notice(msg, args...) do {					\
	_peak_bug(LOG_NOTICE, WHERE_HACK msg, ##args);			\
} while (0)

#define info(msg, args...) do {						\
	_peak_bug(LOG_INFO, WHERE_HACK msg, ##args);			\
} while (0)

#define debug(msg, args...) do {					\
	_peak_bug(LOG_DEBUG, WHERE_HACK msg, ##args);			\
} while (0)

#define output_bump(priority) do {					\
	switch (__sync_fetch_and_or(&(priority), 0)) {			\
	case LOG_EMERG:							\
		/* FALLTHROUGH */					\
	case LOG_ALERT:							\
		/* FALLTHROUGH */					\
	case LOG_CRIT:							\
		/* FALLTHROUGH */					\
	case LOG_ERR:							\
		/* FALLTHROUGH */					\
	case LOG_WARNING:						\
		/* FALLTHROUGH */					\
	case LOG_NOTICE:						\
		/* FALLTHROUGH */					\
	case LOG_INFO:							\
		__sync_fetch_and_add(&(priority), 1);			\
	case LOG_DEBUG:							\
		break;							\
	default:							\
		panic("impossible priority: %d\n", priority);		\
		/* NOTREACHED */					\
	}								\
} while (0)

#define output_init()							\
	int _peak_bug_priority = LOG_EMERG

#define output_bug()	output_bump(_peak_bug_priority)

#endif /* !PEAK_OUTPUT_H */
