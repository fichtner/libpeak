LIB=		netmap
SRCS=		nm_util.c

CFLAGS=		-g -m64 -std=gnu99 -fPIC
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/contrib/libnetmap