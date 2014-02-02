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

#include <fcntl.h>
#include <peak.h>
#include <unistd.h>

struct pcap_file_header {
	uint32_t magic_number;
	uint16_t version_major;
	uint16_t version_minor;
	int32_t thiszone;
	uint32_t sigfigs;
	uint32_t snaplen;
	uint32_t network;
};

struct pcap_packet_header {
	uint32_t ts_sec;
	uint32_t ts_usec;
	uint32_t incl_len;
	uint32_t orig_len;
};

unsigned int
peak_store_packet(int fd, const void *buf, const unsigned int len,
    const int64_t ts_ms)
{
	struct pcap_packet_header hdr = {
		.ts_sec = ts_ms / 1000,
		.incl_len = len,
		.orig_len = len,
	};

	/* calculate subseconds away from initialisation */
	hdr.ts_usec = (ts_ms - (hdr.ts_sec * 1000)) * 1000;

	if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		return (0);
	}

	if (write(fd, buf, len) != len) {
		return (0);
	}

	return (1);
}

int
peak_store_init(const char *file)
{
	struct pcap_file_header hdr = {
		.network = LINKTYPE_ETHERNET,
		.magic_number = 0xA1B2C3D4,
		.version_major = 2,
		.version_minor = 4,
		.snaplen = 65535,
		.thiszone = 0,
		.sigfigs = 0,
	};
	int fd;

	fd = open(file, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0) {
		goto peak_store_init_out;
	}

	if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		goto peak_store_init_close;
	}

	return (fd);

peak_store_init_close:

	close(fd);

peak_store_init_out:

	return (-1);
}

void
peak_store_exit(int fd)
{
	if (fd < 0) {
		return;
	}

	close(fd);
}
