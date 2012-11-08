PROG=		base
SRCS=		base.c

CFLAGS=		-g -m64
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I../../include

LDADD=		-lc -pthread
NOMAN=
