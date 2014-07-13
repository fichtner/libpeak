# -*- makefile -*-

all: $(REGRESS_TARGET)

.SUFFIXES: .in

.in:
	@$(.CURDIR)/../../test/$(REGRESS_FILE)/$(REGRESS_FILE) $*.in | \
	    diff -au $*.out - || \
	    (echo "$* failed" && false)
