LIB=		peak

SRCS=		peak_li.c peak_load.c peak_tcp.c peak_track.c \
		peak_udp.c peak_netmap.c

MAN=		peak_li.3 peak_load.3 peak_tcp.3 peak_track.3 \
		peak_udp.3 peak_netmap.3

CFLAGS=		-g -m64 -std=gnu99 -fPIC
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/include -I$(BASEDIR)/lib
