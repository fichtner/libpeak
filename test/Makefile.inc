BINDIR=	/usr/test

CFLAGS+=-g -m64 -std=gnu99 -Wall -Wextra -Werror
CFLAGS+=-I$(.CURDIR)/../../include -I$(.CURDIR)/../../lib

beforeinstall:
	mkdir -p $(DESTDIR)$(BINDIR)
