MKDEP=	mkdep -p
YACC=	yacc -d
LEX=	flex
CC=	gcc

__YACCINTM=	$(filter %.y,$(SRCS))
_YACCINTM=	$(__YACCINTM:.y=.c)
__LEXINTM=	$(filter %.l,$(SRCS))
_LEXINTM=	$(__LEXINTM:.l=.c)
_SRCS=		$(filter %.c,$(SRCS))

all: .depend $(PROG)

-include .depend

.depend:
ifdef _SRCS
	@$(MKDEP) $(CFLAGS) $(_SRCS)
else
	@touch $@
endif

%.c: %.l
	$(LEX) $<
	@mv lex.yy.c $@

%.c: %.y
	$(YACC) $<
	@mv y.tab.c $@

$(PROG): $(SRCS) $(DPADD) $(_YACCINTM) $(_LEXINTM)
	$(CC) $(CFLAGS) $(_SRCS) $(_YACCINTM) $(_LEXINTM) -o $@ $(LDADD)

clean:
	@$(RM) $(PROG) $(_YACCINTM) $(_LEXINTM)
	@$(RM) .depend y.tab.h
	@$(RM) -r *.dSYM

install:

.PHONY: clean
