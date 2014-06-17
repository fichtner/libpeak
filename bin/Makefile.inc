BINDIR=	/usr/local/sbin

CFLAGS+=-g -m64 -std=gnu99 -Wall -Wextra
CFLAGS+=-I$(.CURDIR)/../../include -I$(.CURDIR)/../../lib

_OSTYPE!=uname -s

.if ${_OSTYPE} == "FreeBSD"
LDADD+=	-lexecinfo
.endif

beforeinstall:
	mkdir -p $(DESTDIR)$(BINDIR)
