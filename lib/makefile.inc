LIB=		peak
SRCS=		peak_li.c peak_load.c peak_tcp.c peak_track.c peak_udp.c \
		peak_netmap.c

MAN+=		peak_load.3
MAN+=		peak_tcp.3
MAN+=		peak_udp.3
MAN+=		peak_li.3
MAN+=		peak_track.3
MAN+=		peak_netmap.3

CFLAGS=		-g -m64 -std=gnu99
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/include -I$(BASEDIR)/lib
