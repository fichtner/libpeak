# -*- makefile -*-

BINDIR?=/usr/local/test

WARNINGS_SET?=	MEDIUM
WARNS?=		6

CFLAGS+=-g -m64 -std=gnu99
CFLAGS+=-I$(.CURDIR)/../../include -I$(.CURDIR)/../../lib

LDADD+=	-L/usr/local/lib

_OSTYPE!=uname -s

.if ${_OSTYPE} == "FreeBSD"
LDADD+=	-lexecinfo
.endif

beforeinstall:
	mkdir -p $(DESTDIR)$(BINDIR)
