#ifndef PEAK_OUTPUT_H
#define PEAK_OUTPUT_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

extern int _peak_bug_priority;
extern int _peak_log_priority;

static inline void
peak_print(FILE *stream, const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	vfprintf(stream, message, ap);
	va_end(ap);
}

/* keep syslog style layout so we can switch on demand later */
#define peak_bug(x, y, args...) do {					\
	if ((x) <= _peak_bug_priority) {				\
		peak_print(stderr, y, ##args);				\
	}								\
} while (0)

/* keep syslog style layout so we can switch on demand later */
#define peak_log(x, y, args...) do {					\
	if ((x) <= _peak_log_priority) {				\
		peak_print(stdout, y, ##args);				\
	}								\
} while (0)

/* fast stdout/stderr access macros */
#define peak_out(x, args...)	peak_log(LOG_EMERG, x, ##args)
#define peak_err(x, args...)	peak_bug(LOG_EMERG, x, ##args)

#define BACKTRACE_NORMAL	1
#define BACKTRACE_SIGNAL	3
#define BACKTRACE_MAX		128

#ifdef __OpenBSD__
#define peak_backtrace(x)
#else /* !__OpenBSD__ */
#include <execinfo.h>

static inline void
peak_backtrace(const int skip)
{
	void *callstack[BACKTRACE_MAX];
	int frames;

	peak_bug(LOG_EMERG, "====== stack trace begin ======\n");
	frames = backtrace(callstack, BACKTRACE_MAX) - skip;
	backtrace_symbols_fd(&callstack[skip], frames, 2);
	peak_bug(LOG_EMERG, "======= stack trace end =======\n");
}
#endif /* __OpenBSD__ */

#undef BACKTRACE_MAX

/* fiddle around with macros to make panics show file/line info */
#define _STRING_HACK(x)	#x
#define STRING_HACK(x)	_STRING_HACK(x)
#define WHERE_HACK	__FILE__ ":" STRING_HACK(__LINE__) ": "

#define peak_panic(__message__, args...) do {				\
	peak_bug(LOG_EMERG, WHERE_HACK __message__, ##args);		\
	peak_backtrace(BACKTRACE_NORMAL);				\
	abort();							\
} while (0)

#define peak_alert(__message__, args...) do {				\
	peak_bug(LOG_ALERT, WHERE_HACK __message__, ##args);		\
} while (0)

#define peak_crit(__message__, args...) do {				\
	peak_bug(LOG_CRIT, WHERE_HACK __message__, ##args);		\
} while (0)

#define peak_error(__message__, args...) do {				\
	peak_bug(LOG_ERR, WHERE_HACK __message__, ##args);		\
} while (0)

#define peak_warn(__message__, args...) do {				\
	peak_bug(LOG_WARNING, WHERE_HACK __message__, ##args);		\
} while (0)

#define peak_notice(__message__, args...) do {				\
	peak_bug(LOG_NOTICE, WHERE_HACK __message__, ##args);		\
} while (0)

#define peak_info(__message__, args...) do {				\
	peak_bug(LOG_INFO, WHERE_HACK __message__, ##args);		\
} while (0)

#define peak_debug(__message__, args...) do {				\
	peak_bug(LOG_DEBUG, WHERE_HACK __message__, ##args);		\
} while (0)

#define peak_priority_bump(__priority__) do {				\
	switch (__priority__) {						\
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
		++(__priority__);					\
	case LOG_DEBUG:							\
		break;							\
	default:							\
		peak_panic("impossible priority: %d\n", __priority__);	\
		/* NOTREACHED */					\
	}								\
} while (0)

#define peak_priority_init()						\
	int _peak_bug_priority = LOG_EMERG;				\
	int _peak_log_priority = LOG_EMERG

#define peak_priority_log() peak_priority_bump(_peak_log_priority)
#define peak_priority_bug() peak_priority_bump(_peak_bug_priority)

#endif /* !PEAK_OUTPUT_H */
