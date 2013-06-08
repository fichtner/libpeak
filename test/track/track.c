#include <peak.h>
#include <assert.h>

output_init();

static void
test_track(void)
{
	struct peak_tracks *tracker;
	struct netmap usr1, usr2;
	struct peak_track _flow;
	struct peak_track *flow;

	netmap4(&usr1, 0);
	netmap4(&usr2, 1);

	tracker = peak_track_init(2);
	assert(tracker);

	TRACK_KEY(&_flow, usr1, usr2, 80, 51000, 0);
	assert(flow = peak_track_acquire(tracker, &_flow));
	assert(flow == peak_track_acquire(tracker, &_flow));
	assert(flow == peak_track_acquire(tracker, flow));

	TRACK_KEY(&_flow, usr2, usr1, 51000, 80, 0);
	assert(flow == peak_track_acquire(tracker, &_flow));

	TRACK_KEY(&_flow, usr1, usr2, 51000, 80, 0);
	assert(flow != peak_track_acquire(tracker, &_flow));

	TRACK_KEY(&_flow, usr1, usr2, 51000, 80, 1);
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
