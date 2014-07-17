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

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif /* !_BSD_SOURCE */

#include <sys/cdefs.h>
#include <sys/types.h>

/* pull internal sys headers for portability */
#include "sys/queue.h"
#include "sys/tree.h"

#define lengthof(x)	(sizeof(x)/sizeof((x)[0]))
#define segv()		*((volatile int *)0)
#define msleep(x)	usleep((x)*1000)
#define strsize(x)	(strlen(x) + 1)
#define stackptr(x)	x[1]

#ifndef unlikely
#define unlikely(exp)	__builtin_expect(!!(exp), 0)
#endif /* !unlikely */

#ifndef likely
#define likely(exp)	__builtin_expect(!!(exp), 1)
#endif /* !likely */

#ifndef __aligned
#define __aligned(x)	__attribute__((aligned(x)))
#endif /* !__aligned */

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

#ifndef XOR
#define XOR(a,b)	(!!(a)+!!(b)==1)
#endif /* !XOR */

/* base headers */
#include "peak_type.h"
#include "peak_sys.h"
#include "peak_output.h"
#include "peak_alloc.h"
#include "peak_prealloc.h"
#include "peak_timeslice.h"
#include "peak_stash.h"
#include "peak_page.h"
#include "peak_net.h"
#include "peak_hash.h"

/* library headers */
#include "peak_locate.h"
#include "peak_load.h"
#include "peak_store.h"
#include "peak_netmap.h"
#include "peak_stream.h"
#include "peak_jar.h"
#include "peak_packet.h"
#include "peak_li.h"
#include "peak_magic.h"
#include "peak_string.h"
#include "peak_track.h"
