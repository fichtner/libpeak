PROG=		peek
SRCS=		peek.c

CFLAGS=		-g -m64 -std=gnu99
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/include -I$(BASEDIR)/lib

LDADD=		-lc -pthread -L$(BASEDIR)/lib -lpeak
DPADD=		$(BASEDIR)/lib/libpeak.a
