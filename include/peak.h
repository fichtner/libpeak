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

#ifdef __linux__
#define strlcpy(x,y,z) do {					\
	if (z) {						\
		memcpy(x, y, (z)-1);				\
		x[(z)-1] = '\0';				\
	}							\
} while (0)
#define strlcat(x,y,z)	strcat(x,y)
#endif /* __linux__ */

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

/* forward declarations */
struct peak_packet;

/* library headers */
#include "peak_load.h"
#include "peak_store.h"
#include "peak_netmap.h"
#include "peak_udp.h"
#include "peak_tcp.h"
#include "peak_li.h"
#include "peak_track.h"
#include "peak_packet.h"
