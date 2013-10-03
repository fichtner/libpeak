MKDEP=	mkdep
CC=	gcc
AR=	ar

_LIBS=	lib$(LIB).a
OBJS=	$(SRCS:.c=.o)

all: .depend $(_LIBS)

-include .depend

.depend:
	@$(MKDEP) $(CFLAGS) $(SRCS)

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

%.a: $(OBJS)
	$(AR) crus $@ $^

clean:
	@$(RM) $(OBJS) $(_LIBS) .depend
	@$(RM) -r *.dSYM

.PHONY: clean
