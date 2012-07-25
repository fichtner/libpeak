CC=gcc
CFLAGS=-g -Wall -Wextra -Iinclude -m64
LDLIBS=-lc -pthread

all: bin/utils

bin/%.d: test/%.c
	$(CC) $(CFLAGS) -MM -MT '$(@:.d=)' $< -MF $@

-include bin/utils.d

bin/%: test/%.c
	$(CC) $(CFLAGS) $(LDLIBS) $< -o $@
	@bin/utils

clean:
	@rm -rf bin/*

# Ubunu 12.04 needs this...
GCCDIR=/usr/lib/gcc/x86_64-linux-gnu/4.6

check: test/utils.c
	@sparse -ftabstop=4 -Wsparse-all -Wno-transparent-union $(CFLAGS) \
	-Wno-declaration-after-statement -Wno-cast-truncate -gcc-base-dir \
	$(GCCDIR) -D__CHECKER__ $^
