PROG=		fuse
SRCS=		fuse.c

CFLAGS=		-g -m64 -std=gnu99
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/include -I$(BASEDIR)/lib

LDADD=		-lc -pthread -L$(BASEDIR)/lib -lpeak
LDADD+=		-L$(BASEDIR)/contrib/libnetmap -lnetmap
DPADD=		$(BASEDIR)/lib/libpeak.a
DPADD+=		$(BASEDIR)/contrib/libnetmap/libnetmap.a
