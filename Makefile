CC=gcc
CFLAGS=-g -Wall -Wextra -Iinclude
LDLIBS=-lc

all: bin/utils

bin/%.d: test/%.c
	$(CC) $(CFLAGS) -MM -MT '$<' $< -MF $@

bin/%: test/%.c
	$(CC) $(CFLAGS) $(LDLIBS) $< -o $@
	@bin/utils

clean:
	@rm -rf bin/* test/*.d
