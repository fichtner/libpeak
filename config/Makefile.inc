.SUFFIXES:	.conf
all: $(CONFS)

.conf:
	@$(.CURDIR)/../../bin/$(CONV)/$(CONV) $<

clean:
	@cd $(.CURDIR)/.. && rm -f $(CONFS:=.conf.bin)

.PHONY: clean
