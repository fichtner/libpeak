MKDEP=	mkdep
ECHO=	echo
CC=	gcc
AR=	ar

ifdef LIB
_LIBS=	lib$(LIB).a
OBJS=	$(SRCS:.c=.o)
endif

all: .depend $(_LIBS) $(PROG)

-include .depend

ifdef PROG
MKDEP	+= -p
endif

.depend:
	@$(MKDEP) $(CFLAGS) $(SRCS)

%.o: %.c
	@$(ECHO) "building $(@F)"
	@$(CC) $(CFLAGS) $< -c -o $@

%.a: $(OBJS)
	@$(ECHO) "bundling $(@F)"
	@$(AR) crus $@ $^

$(PROG): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDADD)

clean:
	@$(RM) $(OBJS) $(_LIBS) $(PROG) .depend
	@$(RM) -r *.dSYM

# Ubuntu 12.04 needs this...
GCCDIR=/usr/lib/gcc/x86_64-linux-gnu/4.6

check: $(SRCS)
ifdef SRCS
	@sparse -ftabstop=4 -Wsparse-all -Wno-transparent-union $(CFLAGS) \
	-Wno-declaration-after-statement -Wno-cast-truncate -gcc-base-dir \
	$(GCCDIR) -D__CHECKER__ $^
endif

.PHONY: clean check
