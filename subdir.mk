.PHONY: $(SUBDIR) recurse

BASEDIR?=	$(shell pwd)

$(MAKECMDGOALS) recurse: $(SUBDIR)

$(SUBDIR):
	@exec $(MAKE) -C $@ $(MAKECMDGOALS) BASEDIR=$(BASEDIR)
