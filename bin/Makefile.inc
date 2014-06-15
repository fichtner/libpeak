BINDIR=	/usr/local/sbin

CFLAGS+=-g -m64 -std=gnu99 -Wall -Wextra
CFLAGS+=-I$(.CURDIR)/../../include -I$(.CURDIR)/../../lib

beforeinstall:
	mkdir -p $(DESTDIR)$(BINDIR)
