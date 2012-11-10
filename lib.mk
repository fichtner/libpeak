MKDEP=	mkdep
ECHO=	echo
CC=	gcc
AR=	ar

_LIBS=	lib$(LIB).a
OBJS=	$(SRCS:.c=.o)

all: .depend $(_LIBS)

-include .depend

.depend:
	@$(MKDEP) $(CFLAGS) $(SRCS)

%.o: %.c
	@$(ECHO) "building $(@F)"
	@$(CC) $(CFLAGS) $< -c -o $@

%.a: $(OBJS)
	@$(ECHO) "bundling $(@F)"
	@$(AR) crus $@ $^

clean:
	@$(RM) $(OBJS) $(_LIBS) .depend
	@$(RM) -r *.dSYM

.PHONY: clean
