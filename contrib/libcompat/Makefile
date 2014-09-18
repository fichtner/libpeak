LIB=	compat
SRCS=	md5.c explicit_bzero.c strlcpy.c strlcat.c

WARNS=	6

CFLAGS=	-g -O2 -m64 -std=gnu99 -fPIC

beforeinstall:
	mkdir -p $(DESTDIR)$(LIBDIR)

.include <bsd.lib.mk>
