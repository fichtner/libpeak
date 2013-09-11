/*
 * Copyright (c) 2013 Franco Fichtner <franco@packetwerk.com>
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
#include <signal.h>

output_init();

static int loop = 1;

static void
fuse_signal(int sig)
{
	(void)sig;
	loop = 0;
	pout("\b\b");
}

static void
usage(void)
{
	extern char *__progname;

	perr("usage: %s dev0 dev1\n", __progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	const char *dev0, *dev1;
	struct peak_netmap *pkt;

	if (argc < 3) {
		usage();
		/* NOTREACHED */
	}

	signal(SIGINT, fuse_signal);

	output_bug();

	dev0 = argv[1];
	dev1 = argv[2];

	if (peak_netmap_attach(dev0)) {
		perr("could not attach to %s\n", dev0);
		return (1);
	}

	if (peak_netmap_attach(dev1)) {
		perr("could not attach to %s\n", dev1);
		peak_netmap_detach(dev0);
		return (1);
	}

	peak_netmap_lock();

	while (loop) {
		pkt = peak_netmap_claim(200);
		if (pkt) {
			if (peak_netmap_forward(pkt,
			    !strcmp(dev0, pkt->ifname) ? dev1 : dev0)) {
				perr("%lld: dropping packet of size %u\n",
				    pkt->ts_unix, pkt->len);
				peak_netmap_forward(pkt, NULL);
			}
		}
	}

	peak_netmap_unlock();

	peak_netmap_detach(dev0);
	peak_netmap_detach(dev1);

	return (0);
}
