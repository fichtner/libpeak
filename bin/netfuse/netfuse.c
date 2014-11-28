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
#include <signal.h>

output_init();

static int loop = 1;

static void
netfuse_signal(int sig)
{
	(void)sig;
	loop = 0;
	pout("\b\b");
}

static void
usage(void)
{
	fprintf(stderr, "usage: netfuse dev0 dev1\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	const unsigned int mode = NETMAP_WIRE;
	struct peak_transfer stackptr(pkt);
	const char *dev0, *dev1;

	if (argc < 3) {
		usage();
		/* NOTREACHED */
	}

	signal(SIGINT, netfuse_signal);

	output_bug();

	dev0 = argv[1];
	dev1 = argv[2];

	if (transfer_netmap.attach(dev0)) {
		perr("could not attach to %s\n", dev0);
		return (1);
	}

	if (transfer_netmap.attach(dev1)) {
		perr("could not attach to %s\n", dev1);
		transfer_netmap.detach(dev0);
		return (1);
	}

	transfer_netmap.lock();

	while (loop) {
		if (!transfer_netmap.recv(pkt, 200, NULL, mode)) {
			continue;
		}
		if (transfer_netmap.send(pkt, !strcmp(dev0, pkt->ifname) ?
		    dev1 : dev0, mode)) {
			perr("%lld: dropping packet of size %u\n",
			    pkt->ts.tv_sec, pkt->len);
			transfer_netmap.send(pkt, NULL, mode);
		}
	}

	transfer_netmap.unlock();

	transfer_netmap.detach(dev0);
	transfer_netmap.detach(dev1);

	return (0);
}
