all: $(REGRESS_TARGET)

.SUFFIXES: .in

.in:
	@$(FILE) $*.in | \
	    diff -au $*.out - || \
	    (echo "$* failed" && false)
