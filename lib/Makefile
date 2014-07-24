LIB=	peak

SHLIB_MAJOR=1
SHLIB_MINOR=0

SRCS=	peak_li.c peak_load.c peak_track.c peak_packet.c \
	peak_store.c peak_jar.c peak_locate.c \
	peak_stream.c peak_string.c peak_magic.c shlib.c

MAN=	peak_li.3 peak_load.3 peak_track.3 peak_packet.3 \
	peak_store.3 peak_jar.3 peak_locate.3 \
	peak_stream.3 peak_string.3 peak_magic.3

LINTFLAGS+=	-I$(.CURDIR)/../include -I$(.CURDIR)/../lib

WARNINGS_SET?=	MEDIUM
WARNS?=		3

CFLAGS+=-g -m64 -std=gnu99 -fPIC
CFLAGS+=-I$(.CURDIR)/../include -I$(.CURDIR)/../lib
CFLAGS+=-I/usr/local/include

LDADD+=	-lc -pthread -lmagic
LDADD+= -L/usr/local/lib

_OSTYPE!=uname -s

.if ${_OSTYPE} == "FreeBSD"
SRCS+=	peak_netmap.c
MAN+=	peak_netmap.3

LDADD+=	-lexecinfo
.endif

beforeinstall:
	mkdir -p $(DESTDIR)$(LIBDIR)

.include <bsd.lib.mk>
