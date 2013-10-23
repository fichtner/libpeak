MKDEP=	mkdep
CC=	gcc
AR=	ar

_LIBS=	lib$(LIB).a
ifdef SHLIB_MAJOR
_LIBS+=	lib$(LIB).so
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

%.so: $(SRCS)
	$(CC) -shared $(CFLAGS) -o $@ $^ $(LDADD)

clean:
	@$(RM) $(OBJS) $(_LIBS) .depend
	@$(RM) -r *.dSYM

install:

.PHONY: clean
