PROG=		load
SRCS=		load.c
NOMAN=

CFLAGS=		-g -m64
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I../../include -I../../lib

LDADD=		-lc -pthread -L../../lib -lpeak
DPADD=		../../lib/libpeak.a
