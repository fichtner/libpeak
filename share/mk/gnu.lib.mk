MKDEP=	mkdep
CC=	gcc
AR=	ar

_LIBS=	lib$(LIB).a
ifdef SHLIB_MAJOR
ifdef SHLIB_MINOR
SHLIB=	lib$(LIB).so.$(SHLIB_MAJOR).$(SHLIB_MINOR)
_LIBS+=	$(SHLIB)
endif
endif
OBJS=	$(SRCS:.c=.o)

ifdef LIB
all: .depend $(_LIBS)
else
all:
endif

-include .depend

.depend:
ifdef SRCS
	@$(MKDEP) $(CFLAGS) $(SRCS)
else
	@touch $@
endif

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

%.a: $(OBJS)
	$(AR) crus $@ $^

ifdef SHLIB
$(SHLIB): $(SRCS)
	$(CC) -shared $(CFLAGS) -o $@ $^ $(LDADD)
endif

clean:
	@$(RM) $(OBJS) $(_LIBS) .depend
	@$(RM) -r *.dSYM

install:

.PHONY: clean
