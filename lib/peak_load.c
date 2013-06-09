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
} __packed;

struct pcap_packet_header {
	uint32_t ts_sec;
	uint32_t ts_usec;
	uint32_t incl_len;
	uint32_t orig_len;
} __packed;

struct erf_packet_header {
	uint64_t ts;
	uint8_t type;
	uint8_t flags;
	uint16_t rlen;
	uint16_t lctr;
	uint16_t wlen;
	uint16_t wtf;
} __packed;

#define PCAP_MS(x, y)	((x) * 1000 + (y) / 1000)
#define PCAP_MAGIC	0xA1B2C3D4
#define PCAP_FMT	1

#define ERF_MS(x)	((((x) >> 32) * 1000) +				\
    ((((uint32_t)(x)) * 1000ull) >> 32))
#define ERF_FMT		0

static inline uint32_t
peak_load_normalise(struct peak_load *self, uint32_t ts_ms)
{
	ts_ms += self->ts_off;

	if (likely(!wrap32(ts_ms - self->ts_ms))) {
		return (ts_ms);
	}

	/* went back in time! */
	self->ts_off += self->ts_ms - ts_ms;

	return (self->ts_ms);
}

static inline void
_peak_load_erf(struct peak_load *self)
{
	struct erf_packet_header hdr;
	ssize_t ret;

	ret = read(self->fd, &hdr, sizeof(hdr));
	if (ret != sizeof(hdr)) {
		return;
	}

	hdr.rlen = be16dec(&hdr.rlen) - sizeof(hdr);
	hdr.wlen = be16dec(&hdr.wlen);

	if (sizeof(self->buf) < hdr.rlen) {
		return;
	}

	ret = read(self->fd, self->buf, hdr.rlen);
	if (ret != hdr.rlen) {
		return;
	}

	self->ts_ms = peak_load_normalise(self, ERF_MS(hdr.ts));
	self->ts_unix = hdr.ts >> 32;
	self->len = hdr.wlen;
}

static inline void
_peak_load_pcap(struct peak_load *self)
{
	struct pcap_packet_header hdr;
	ssize_t ret;

	ret = read(self->fd, &hdr, sizeof(hdr));
	if (ret != sizeof(hdr)) {
		return;
	}

	if (sizeof(self->buf) < hdr.incl_len) {
		return;
	}

	ret = read(self->fd, self->buf, hdr.incl_len);
	if (ret != hdr.incl_len) {
		return;
	}

	self->ts_ms = peak_load_normalise(self,
	    PCAP_MS(hdr.ts_sec, hdr.ts_usec));
	self->ts_unix = hdr.ts_sec;
	self->len = hdr.incl_len;
}

unsigned int
peak_load_next(struct peak_load *self)
{
	self->len = 0;

	switch (self->fmt) {
	case ERF_FMT:
		_peak_load_erf(self);
		break;
	case PCAP_FMT:
		_peak_load_pcap(self);
		break;
	}

	return (self->len);
}

struct peak_load *
peak_load_init(const char *file)
{
	struct peak_load *self = NULL;
	unsigned int fmt = ERF_FMT;
	int fd = STDIN_FILENO;

	if (file) {
		struct pcap_file_header hdr;

		fd = open(file, O_RDONLY);
		if (fd < 0) {
			goto peak_load_init_out;
		}

		/* autodetect pcap if possible */
		if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
			goto peak_load_init_close;
		}

		if (hdr.magic_number == PCAP_MAGIC) {
			fmt = PCAP_FMT;
		} else {
			/* rewind & pretend nothing happened */
			lseek(fd, 0, SEEK_SET);
		}
	}

	self = zalloc(sizeof(*self));
	if (!self) {
		goto peak_load_init_close;
	}

	self->fmt = fmt;
	self->fd = fd;

	return (self);

  peak_load_init_close:

	if (fd != STDIN_FILENO) {
		close(fd);
	}

  peak_load_init_out:

	return (NULL);
}

void
peak_load_exit(struct peak_load *self)
{
	if (self) {
		if (self->fd != STDIN_FILENO) {
			close(self->fd);
		}
		free(self);
	}
}
