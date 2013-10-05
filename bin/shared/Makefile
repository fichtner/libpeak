CFLAGS=		-g -m64 -std=gnu99 -fPIC
CFLAGS+=	-Wall -Wextra -Werror

CFLAGS+=	-I../../include -I../../lib
SRCS=		../../lib/peak_load.c
SRCS+=		../../lib/peak_tcp.c
SRCS+=		../../lib/peak_udp.c
SRCS+=		../../lib/peak_li.c
SRCS+=		../../lib/peak_track.c

all: libpeak.so

libpeak.so:
	gcc -shared $(CFLAGS) -o $@ $(SRCS) shared.c -pthread

install:
manlint:
check:

clean:
	@rm -rf libpeak.so*

.PHONY: clean
