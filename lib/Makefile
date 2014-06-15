LIB=	peak

SHLIB_MAJOR=1
SHLIB_MINOR=0

SRCS=	peak_li.c peak_load.c peak_track.c peak_packet.c \
	peak_netmap.c peak_store.c peak_jar.c \
	peak_stream.c peak_string.c shlib.c

MAN=	peak_li.3 peak_load.3 peak_track.3 peak_packet.3 \
	peak_netmap.3 peak_store.3 peak_jar.3 \
	peak_stream.3 peak_string.3

CFLAGS+=-g -m64 -std=gnu99 -fPIC -Wall -Wextra -Werror
CFLAGS+=-I$(.CURDIR)/../include -I$(.CURDIR)/../lib

LDADD+=	-lc -pthread -lmagic

_OSTYPE!=uname -s

.if ${_OSTYPE} == "FreeBSD"
LDADD+=	-lexecinfo
.endif

beforeinstall:
	mkdir -p $(DESTDIR)$(LIBDIR)

.include <bsd.lib.mk>
