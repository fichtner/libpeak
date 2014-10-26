# -*- makefile -*-

all: $(REGRESS_TEST)

.SUFFIXES: .in

.in:
	@$(.CURDIR)/../../$(REGRESS_TYPE)/$(REGRESS_FILE)/$(REGRESS_FILE) \
	    $*.in | diff -au $*.out - || (echo "$* failed" && false)
