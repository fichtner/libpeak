CFLAGS=	-g -Wall -Wextra -I../include -m64
LDLIBS=	-lc -pthread

ECHO=	echo
CC=		gcc
AR=		ar

SRCS=	$(wildcard *.c)
DEPS=	$(SRCS:.c=.d)
ifdef LIB
OBJS=	$(SRCS:.c=.o)
endif

all: $(LIB) $(PROG)

regress:
ifdef PROG
	@for test in $(PROG); do (exec $(ECHO) | ./$$test); done
endif

%.d: %.c
ifdef PROG
	@$(CC) $(CFLAGS) -MM -MT '$*' $< -MF $@
else
	@$(CC) $(CFLAGS) -MM -MT '$*.o' $< -MF $@
endif

-include $(DEPS)

%.o: %.c
	@$(ECHO) "building $(@F)"
	@$(CC) $(CFLAGS) $< -c -o $@

%.a: $(OBJS)
	@$(ECHO) "bundling $(@F)"
	@$(AR) crus $@ $^

$(PROG): $(SRCS)
	$(CC) $(CFLAGS) $@.c -o $@ $(LDLIBS)

clean:
	@$(RM) $(OBJS) $(DEPS) $(LIB) $(PROG)
	@$(RM) -r *.dSYM

# Ubuntu 12.04 needs this...
GCCDIR=/usr/lib/gcc/x86_64-linux-gnu/4.6

check: $(SRCS)
ifdef SRCS
	@sparse -ftabstop=4 -Wsparse-all -Wno-transparent-union $(CFLAGS) \
	-Wno-declaration-after-statement -Wno-cast-truncate -gcc-base-dir \
	$(GCCDIR) -D__CHECKER__ $^
endif

.PHONY: clean check regress
