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
#include <limits.h>
#include <unistd.h>

output_init();

static unsigned int flow_count = 10000;
static unsigned int file_count = 2;

static int64_t
flowsplit_packet(struct peak_tracks *track, void *buf,
    unsigned int len, unsigned int type)
{
	struct peak_packet stackptr(packet);
	struct peak_track *flow;
	struct peak_track _flow;

	if (peak_packet_parse(packet, buf, len, type) || !packet->net_len) {
		/* packet couldn't be parsed */
		return (-1);
	}

	TRACK_KEY(&_flow, packet);

	flow = peak_track_acquire(track, &_flow);
	if (!flow) {
		panic("tracker is empty\n");
	}

	return (flow->id);
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: flowsplit [-f flow_count] [-n file_count] file\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	struct peak_tracks *track;
	struct peak_load *trace;
	unsigned int i;
	char *dot_fn;
	int *fds;
	int c;

	while ((c = getopt(argc, argv, "n:f:")) != -1) {
		switch (c) {
		case 'f':
			flow_count = atoi(optarg);
			break;
		case 'n':
			file_count = atoi(optarg);
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage();
		/* NOTREACHED */
	}

	trace = peak_load_init(argv[0]);
	if (!trace) {
		panic("cannot init file loader\n");
	}

	track = peak_track_init(flow_count, 1);
	if (!track) {
		panic("cannot init flow tracker\n");
	}

	fds = reallocarray(NULL, file_count, sizeof(*fds));
	if (!fds) {
		panic("cannot allocate file descriptor memory\n");
	}

	if (!file_count) {
		panic("one file per flow not yet implemented\n");
	}

	file_count = MIN(file_count, flow_count);

	dot_fn = strrchr(argv[0], '.');
	if (dot_fn) {
		*dot_fn = '\0';
		++dot_fn;
	}

	for (i = 0; i < file_count; ++i) {
		char split_fn[PATH_MAX];

		snprintf(split_fn, sizeof(split_fn), "%s_%u.pcap",
		    argv[0], i);

		fds[i] = peak_store_init(split_fn);
		if (fds[i] < 0) {
			panic("cannot open an output file\n");
		}
	}

	if (peak_load_packet(trace)) {
		do {
			const int64_t ret = flowsplit_packet(track,
			    trace->buf, trace->len, trace->ll);
			if (ret < 0) {
				continue;
			}

			peak_store_packet(fds[ret % file_count],
			    trace->buf, trace->len, trace->ts.tv_sec,
			    trace->ts.tv_usec);
		} while (peak_load_packet(trace));
	}

	for (i = 0; i < file_count; ++i) {
		peak_store_exit(fds[i]);
	}

	free(fds);

	peak_track_exit(track);
	peak_load_exit(trace);

	return (0);
}
