REGRESS_FILE=	peek
REGRESS_TYPE=	bin

#	output			sample
REGRESS_TEST= \
	test_erf		test.erf \
	test_pcap		test.pcap \
	test_pcap_swap		test_be.pcap \
	test_pcapng		test.pcapng \

all:
	@set `echo $(REGRESS_TEST)`; \
	while : ; do \
		case $$# in \
		0) break;; \
		1) echo "$$1 failed: missing arguments"; exit 1;; \
		esac; \
		TRACE=$(.CURDIR)/../../sample/$$2; \
		[ ! -f $$TRACE ] && \
		    echo "$$1 failed: $$TRACE not found" && exit 1; \
		$(.CURDIR)/../../$(REGRESS_TYPE)/$(REGRESS_FILE)/$(REGRESS_FILE) \
		    $$TRACE | diff -au $$1 - || (echo "$$1 failed" && exit 1); \
		shift; shift; \
	done
