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
#include <assert.h>

output_init();

static void
test_track(void)
{
	struct peak_tracks *tracker;
	struct peak_packet packet;
	struct netaddr usr1, usr2;
	struct peak_track _flow;
	struct peak_track *flow;

	memset(&packet, 0, sizeof(packet));

	netaddr4(&usr1, 0);
	netaddr4(&usr2, 1);

	tracker = peak_track_init(2);
	assert(tracker);

	packet.net_saddr = usr1;
	packet.net_daddr = usr2;
	packet.flow_sport = 80;
	packet.flow_dport = 51000;
	TRACK_KEY(&_flow, &packet);
	flow = peak_track_acquire(tracker, &_flow);

	assert(flow);
	assert(flow == peak_track_acquire(tracker, &_flow));
	assert(flow == peak_track_acquire(tracker, flow));

	packet.net_saddr = usr2;
	packet.net_daddr = usr1;
	packet.flow_sport = 51000;
	packet.flow_dport = 80;
	TRACK_KEY(&_flow, &packet);
	assert(flow == peak_track_acquire(tracker, &_flow));

	packet.net_saddr = usr1;
	packet.net_daddr = usr2;
	packet.flow_sport = 51000;
	packet.flow_dport = 80;
	TRACK_KEY(&_flow, &packet);
	assert(flow != peak_track_acquire(tracker, &_flow));

	packet.net_saddr = usr1;
	packet.net_daddr = usr2;
	packet.flow_sport = 51000;
	packet.flow_dport = 80;
	packet.net_type = 1;
	TRACK_KEY(&_flow, &packet);
	assert(peak_track_acquire(tracker, &_flow));

	peak_track_exit(tracker);
}

int
main(void)
{
	pout("peak track test suite... ");

	test_track();

	pout("ok\n");

	return (0);
}
