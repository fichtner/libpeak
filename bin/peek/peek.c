/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
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

output_init();

enum {
	USE_APP,
	USE_APP_LEN,
	USE_FLOW,
	USE_IP_LEN,
	USE_IP_TYPE,
	USE_TIME,
	USE_MAX		/* last element */
};

static unsigned int use_print[USE_MAX];
static unsigned int use_count = 0;

static void
peek_report(const struct peak_packet *packet, const struct peak_track *flow,
    const timeslice_t *timer)
{
	unsigned int i;

	for (i = 0; i < use_count; ++i) {
		if (i) {
			pout(", ");
		}

		switch (use_print[i]) {
		case USE_APP:
			pout("app: %s", peak_li_name(peak_li_merge(flow->li)));
			break;
		case USE_APP_LEN:
			pout("app_len: %hu", packet->app_len);
			break;
		case USE_FLOW:
			pout("flow: %llu", (unsigned long long)flow->id);
			break;
		case USE_IP_LEN:
			pout("ip_len: %hu", packet->net_len);
			break;
		case USE_IP_TYPE:
			pout("ip_type: %hhu", packet->net_type);
			break;
		case USE_TIME: {
			char tsbuf[40];
			pout("time: %s", strftime(tsbuf, sizeof(tsbuf),
			    "%a %F %T", &timer->gmt) ? tsbuf : "???");
			break;
		}
		default:
			break;
		}
	}

	pout("\n");
}

static void
peek_packet(struct peak_tracks *peek, const timeslice_t *timer,
    void *buf, unsigned int len, unsigned int type)
{
	struct peak_packet stackptr(packet);
	struct peak_track *flow;
	struct peak_track _flow;

	if (peak_packet_parse(packet, buf, len, type) || !packet->net_len) {
		/* here be dragons */
		return;
	}

	TRACK_KEY(&_flow, packet);

	flow = peak_track_acquire(peek, &_flow);
	if (!flow) {
		panic("tracker is empty\n");
	}

	{
		const unsigned int dir =
		    !!netcmp(&packet->net_saddr, &flow->addr[LOWER]);

		if (!flow->li[dir]) {
			flow->li[dir] = peak_li_get(packet);
		}
	}

	peek_report(packet, flow, timer);
}

static void
usage(void)
{
	fprintf(stderr, "usage: peek [-AafNnt] file\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	struct peak_tracks *peek;
	struct peak_load *trace;
	timeslice_t timer;
	int c;

	while ((c = getopt(argc, argv, "AafNnt")) != -1) {
		switch (c) {
		case 'A':
			use_print[use_count++] = USE_APP_LEN;
			break;
		case 'a':
			use_print[use_count++] = USE_APP;
			break;
		case 'f':
			use_print[use_count++] = USE_FLOW;
			break;
		case 'N':
			use_print[use_count++] = USE_IP_LEN;
			break;
		case 'n':
			use_print[use_count++] = USE_IP_TYPE;
			break;
		case 't':
			use_print[use_count++] = USE_TIME;
			break;
		default:
			usage();
			/* NOTREACHED */
		}

		if (use_count > USE_MAX) {
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

	if (!use_count) {
		/* set the default output (as used by tests) */
		use_print[use_count++] = USE_FLOW;
		use_print[use_count++] = USE_IP_TYPE;
		use_print[use_count++] = USE_IP_LEN;
		use_print[use_count++] = USE_APP;
		use_print[use_count++] = USE_TIME;
	}

	trace = peak_load_init(argv[0]);
	if (!trace) {
		panic("cannot init file loader\n");
	}

	peek = peak_track_init(10000, 1);
	if (!peek) {
		panic("cannot init flow tracker\n");
	}

	TIMESLICE_INIT(&timer);

	if (peak_load_packet(trace)) {
		TIMESLICE_CALIBRATE(&timer, &trace->ts);

		do {
			TIMESLICE_ADVANCE(&timer, &trace->ts);
			peek_packet(peek, &timer, trace->buf, trace->len,
			    trace->ll);
		} while (peak_load_packet(trace));
	}

	peak_track_exit(peek);
	peak_load_exit(trace);

	return (0);
}
