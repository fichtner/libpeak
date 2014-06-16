/*
 * Copyright (c) 2014 Franco Fichtner <franco@packetwerk.com>
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
#include <unistd.h>
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

static void
test_store(const char *file, const unsigned int *len, const size_t count)
{
	char template[] = "/tmp/store.XXXXXX";
	struct peak_load *trace;
	int store;
	size_t i;

	trace = peak_load_init(file);
	assert(trace);

	store = peak_store_init(mktemp(template));
	assert(store >= 0);

	for (i = 0; i < count; ++i) {
		assert(peak_load_packet(trace) == len[i]);
		assert(peak_store_packet(store, trace->buf, len[i],
		    trace->ts.tv_sec, trace->ts.tv_usec));
	}

	peak_store_exit(store);
	peak_load_exit(trace);

	trace = peak_load_init(template);
	assert(trace);

	for (i = 0; i < count; ++i) {
		assert(peak_load_packet(trace) == len[i]);
	}

	peak_load_exit(trace);
	unlink(template);
}

int
main(void)
{
	pout("peak store test suite... ");

	test_store(pcap_file, pcap_len, lengthof(pcap_len));

	pout("ok\n");

	return (0);
}
