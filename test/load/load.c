/*
 * Copyright (c) 2012 Franco Fichtner <franco@lastsummer.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <peak.h>

#ifdef __OpenBSD__
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#else /* !__OpenBSD__ */
#include <net/ethernet.h>
#endif /* __OpenBSD__ */
#include <assert.h>

output_init();

static const unsigned int test_pcap_len[] = {
	76, 142, 62, 62, 54, 235, 63, 60, 191, 66, 72,
	84, 72, 72, 84, 90, 62, 93, 68, 60, 110, 1514,
	1514, 1514, 1514, 590, 1506, 590, 590, 590,
	60, 1506, 1506, 60, 1506, 1506, 60, 1506, 1506,
	60, 1506, 1506, 60, 1506, 83, 60, 60, 60, 60,
	60, 60, 82, 54, 60, 54, 102, 60, 54, 60, 243,
};

static void
test_pcap(void)
{
	struct peak_load *pcap = peak_load_init("../../sample/test.pcap");
	struct ether_header *eth;
	unsigned int i;

	assert(pcap);

	eth = (struct ether_header *) pcap->buf;

	for (i = 0; i < lengthof(test_pcap_len); ++i) {
		assert(peak_load_next(pcap) == test_pcap_len[i]);
		assert(be16dec(&eth->ether_type) == ETHERTYPE_IP);
	}

	assert(!peak_load_next(pcap));
	assert(!peak_load_next(pcap));

	peak_load_exit(pcap);
}

static const unsigned int test_erf_len[] = {
	78, 64, 64, 711, 64, 1470, 64, 1470, 64, 393,
	64, 711, 64, 1470, 262, 64, 64, 64, 64,
};

static void
test_erf(void)
{
	struct peak_load *erf = peak_load_init("../../sample/test.erf");
	struct ether_header *eth;
	unsigned int i;

	assert(erf);

	eth = (struct ether_header *) erf->buf;

	for (i = 0; i < lengthof(test_erf_len); ++i) {
		assert(peak_load_next(erf) == test_erf_len[i]);
		assert(be16dec(&eth->ether_type) == ETHERTYPE_IP);
	}

	assert(!peak_load_next(erf));
	assert(!peak_load_next(erf));

	peak_load_exit(erf);
}

int
main(void)
{
	pout("peak load test suite... ");

	test_pcap();
	test_erf();

	pout("ok\n");

	return (0);
}
