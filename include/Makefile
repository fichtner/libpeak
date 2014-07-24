MAN=	peak_alloc.3 peak_hash.3 peak_net.3 peak_output.3 \
	peak_page.3 peak_prealloc.3 peak_stash.3 peak_sys.3 \
	peak_timeslice.3 peak_type.3 peak_transfer.3

beforeinstall:
.for section in $(MAN:E:O:u)
	mkdir -p $(DESTDIR)$(MANDIR)$(section)
.endfor

.include <bsd.prog.mk>
