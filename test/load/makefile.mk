PROG=		load
SRCS=		load.c
NO_MAN=
NOMAN=

CFLAGS=		-g -m64
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/include -I$(BASEDIR)/lib

LDADD=		-lc -pthread -L$(BASEDIR)/lib -lpeak
DPADD=		$(BASEDIR)/lib/libpeak.a
