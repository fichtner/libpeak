PROG=		fuse
SRCS=		fuse.c

BINDIR=		/usr/bin

CFLAGS=		-g -m64 -std=gnu99
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/include -I$(BASEDIR)/lib

LDADD=		-lc -pthread
LDADD+=		$(BASEDIR)/lib/libpeak.a
LDADD+=		$(BASEDIR)/contrib/libnetmap/libnetmap.a
DPADD=		$(BASEDIR)/lib/libpeak.a
DPADD+=		$(BASEDIR)/contrib/libnetmap/libnetmap.a
