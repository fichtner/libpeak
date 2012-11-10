SPARSE=		sparse
SFLAGS=		-ftabstop=4 -Wsparse-all -Wno-transparent-union
SFLAGS+=	-Wno-declaration-after-statement -Wno-cast-truncate
SFLAGS+=	-D__CHECKER__
# Ubuntu 12.04 needs this...
CCDIR=		/usr/lib/gcc/x86_64-linux-gnu/4.6

check: $(SRCS)
	@$(SPARSE) $(SFLAGS) $(CFLAGS) -gcc-base-dir $(CCDIR) $^

.PHONY: check
