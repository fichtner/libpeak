/*
 * Copyright (c) 2012-2013 Franco Fichtner <franco@packetwerk.com>
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

static const unsigned int pcap_len[] = {
	76, 142, 62, 62, 54, 235, 63, 60, 191, 66, 72,
	84, 72, 72, 84, 90, 62, 93, 68, 60, 110, 1514,
	1514, 1514, 1514, 590, 1506, 590, 590, 590,
	60, 1506, 1506, 60, 1506, 1506, 60, 1506, 1506,
	60, 1506, 1506, 60, 1506, 83, 60, 60, 60, 60,
	60, 60, 82, 54, 60, 54, 102, 60, 54, 60, 243,
};

static const char *pcap_file = "../../sample/test.pcap";

static const unsigned int pcapng_len[] = {
	78,
};

static const char *pcapng_file = "../../sample/test.pcapng";

static const unsigned int erf_len[] = {
	78, 64, 64, 711, 64, 1470, 64, 1470, 64, 393,
	64, 711, 64, 1470, 262, 64, 64, 64, 64,
};

static const char *erf_file = "../../sample/test.erf";

static void
test_load(const char *file, const unsigned int *len, const size_t count)
{
	struct peak_load *trace = peak_load_init(file);
	struct ether_header *eth;
	size_t i;

	assert(trace);

	eth = (struct ether_header *)trace->buf;

	for (i = 0; i < count; ++i) {
		assert(peak_load_next(trace) == len[i]);
		assert(be16dec(&eth->ether_type) == ETHERTYPE_IP);
	}

	assert(!peak_load_next(trace));
	assert(!peak_load_next(trace));

	peak_load_exit(trace);
}

int
main(void)
{
	pout("peak load test suite... ");

	test_load(pcap_file, pcap_len, lengthof(pcap_len));
	test_load(pcapng_file, pcapng_len, lengthof(pcapng_len));
	test_load(erf_file, erf_len, lengthof(erf_len));

	pout("ok\n");

	return (0);
}
