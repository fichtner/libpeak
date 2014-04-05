LIB=		peak

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

SRCS=		peak_li.c peak_load.c peak_tcp.c peak_track.c \
		peak_udp.c peak_netmap.c peak_store.c peak_jar.c \
		peak_stream.c peak_string.c shlib.c

MAN=		peak_li.3 peak_load.3 peak_tcp.3 peak_track.3 \
		peak_udp.3 peak_netmap.3 peak_store.3 peak_jar.3 \
		peak_stream.3 peak_string.3

CFLAGS=		-g -m64 -std=gnu99 -fPIC
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/include -I$(BASEDIR)/lib

LDADD=		-lc -pthread
LDADD+=		$(BASEDIR)/contrib/libnetmap/libnetmap.a

DPADD=		$(BASEDIR)/contrib/libnetmap/libnetmap.a
