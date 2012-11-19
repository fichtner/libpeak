#ifndef PEAK_MACRO_H
#define PEAK_MACRO_H

#include <sys/cdefs.h>
#include <sys/queue.h>

#define lengthof(x)	(sizeof(x)/sizeof((x)[0]))
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

#endif /* !PEAK_MACRO_H */
