PROG=		base
SRCS=		base.c
NOMAN=

CFLAGS=		-g -m64
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I../../include -I../../lib

LDADD=		-lc -pthread
