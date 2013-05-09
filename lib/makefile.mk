LIB=		peak
SRCS=		peak_load.c

CFLAGS=		-g -m64
CFLAGS+=	-Wall -Wextra -Werror
CFLAGS+=	-I$(BASEDIR)/include -I$(BASEDIR)/lib
