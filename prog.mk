MKDEP=	mkdep -p
CC=	gcc

all: .depend $(PROG)

-include .depend

.depend:
	@$(MKDEP) $(CFLAGS) $(SRCS)

$(PROG): $(SRCS) $(DPADD)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDADD)

clean:
	@$(RM) $(PROG) .depend
	@$(RM) -r *.dSYM

.PHONY: clean
