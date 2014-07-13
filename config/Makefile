GEOS=	locate

.SUFFIXES:	.csv
all: $(GEOS)

.csv:
	@../bin/geoconv/geoconv $<

install:
manlint:
mandiff:
depend:
check:

clean cleandir:
	@rm -f $(GEOS:=.csv.bin)

.PHONY: clean
