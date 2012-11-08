all: $(REGRESS_TARGET)

.SUFFIXES: .in

.in:
	@$(FILE) $(FLAGS) $*.in | \
		diff -u $*.out - || \
		(echo "XXX $* failed" && false)
