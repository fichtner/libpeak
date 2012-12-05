#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif /* !_BSD_SOURCE */

#include <sys/cdefs.h>
#include <sys/queue.h>

#define lengthof(x)	(sizeof(x)/sizeof((x)[0]))
#define segv()		*((volatile int *)0)
#define msleep(x)	usleep((x)*1000)

#ifndef unlikely
#define unlikely(exp)	__builtin_expect(!!(exp), 0)
#endif /* !unlikely */

#ifndef likely
#define likely(exp)	__builtin_expect(!!(exp), 1)
#endif /* !likely */

#ifndef __packed
#define __packed	__attribute__((__packed__))
#endif /* !__packed */

#ifndef __unused
#define __unused	__attribute__((__unused__))
#endif /* !__unused */

#ifndef MIN
#define MIN(a,b)	(((a)<(b))?(a):(b))
#endif /* !MIN */

#ifndef MAX
#define MAX(a,b)	(((a)>(b))?(a):(b))
#endif /* !MAX */

/* base headers */
#include "peak_type.h"
#include "peak_sys.h"
#include "peak_output.h"
#include "peak_alloc.h"
#include "peak_prealloc.h"
#include "peak_hash.h"
