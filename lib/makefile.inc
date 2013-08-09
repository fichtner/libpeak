LIB=		peak
SRCS=		peak_li.c peak_load.c peak_tcp.c peak_track.c peak_udp.c

CFLAGS=		-g -m64 -std=gnu99
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/include -I$(BASEDIR)/lib
