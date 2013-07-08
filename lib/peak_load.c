/*
 * Copyright (c) 2012 Franco Fichtner <franco@packetwerk.com>
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

struct erf_packet_header {
	uint64_t ts;
	uint8_t type;
	uint8_t flags;
	uint16_t rlen;
	uint16_t lctr;
	uint16_t wlen;
	uint16_t wtf;
} __packed;

#define ERF_MS(x)	((((x) >> 32) * 1000) +				\
    ((((uint32_t)(x)) * 1000ull) >> 32))
#define ERF_FMT		0

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

#define PCAP_MS(x, y)	((x) * 1000 + (y) / 1000)
#define PCAP_MAGIC	0xA1B2C3D4
#define PCAP_FMT	1

struct pcapng_block_header {
	uint32_t type;
	uint32_t length;
};

struct pcapng_iface_desc_header {
	uint16_t linktype;
	uint16_t reserved;
	uint32_t snaplen;
};

struct pcapng_enhanced_pkt_header {
	uint32_t interface_id;
	uint32_t ts_high;
	uint32_t ts_low;
	uint32_t capturelen;
	uint32_t packetlen;
};

#define PCAPNG_MAGIC	0x0A0D0D0A
#define PCAPNG_FMT	2

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

static void
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

static void
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

static void
_peak_load_pcapng(struct peak_load *self)
{
	struct pcapng_enhanced_pkt_header pkt;
	struct pcapng_iface_desc_header iface;
	struct pcapng_block_header hdr;
	ssize_t ret;
	uint64_t ts;

_peak_load_pcapng_again:

	ret = read(self->fd, &hdr, sizeof(hdr));
	if (ret != sizeof(hdr)) {
		return;
	}

	if (sizeof(self->buf) < hdr.length) {
		return;
	}

	switch (hdr.type) {
	case 2: /* packet block (obsolete) */
	case 6:	/* enhanced packet block */
		ret = read(self->fd, &pkt, sizeof(pkt));
		if (ret != sizeof(pkt)) {
			return;
		}

		ret = read(self->fd, self->buf, hdr.length -
		    sizeof(hdr) - sizeof(pkt));
		if (ret != (ssize_t)(hdr.length -
		    sizeof(hdr) - sizeof(pkt))) {
			return;
		}

		ts = (uint64_t)pkt.ts_high << 32 | (uint64_t)pkt.ts_low;

		self->ts_ms = ts / 1000ull;
		self->ts_unix = ts / 1000ull / 1000ull;
		self->len = pkt.packetlen;

		break;
	case 1:	/* interface description block */
		ret = read(self->fd, &iface, sizeof(iface));
		if (ret != sizeof(iface)) {
			return;
		}

		lseek(self->fd, hdr.length - sizeof(hdr) - sizeof(iface),
		    SEEK_CUR);

		self->ll = iface.linktype;

		goto _peak_load_pcapng_again;
	default:
		/*
		 * The blocks we don't know we don't look at more
		 * closely. A new section may begin here as well,
		 * but we mop up the link type change above anyway.
		 */
		lseek(self->fd, hdr.length - sizeof(hdr), SEEK_CUR);
		goto _peak_load_pcapng_again;
	}
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
	case PCAPNG_FMT:
		_peak_load_pcapng(self);
		break;
	}

	return (self->len);
}

struct peak_load *
peak_load_init(const char *file)
{
	unsigned int ll = LINKTYPE_ETHERNET;
	struct peak_load *self = NULL;
	unsigned int fmt = ERF_FMT;
	int fd = STDIN_FILENO;

	if (file) {
		uint32_t file_magic;

		fd = open(file, O_RDONLY);
		if (fd < 0) {
			goto peak_load_init_out;
		}

		/* autodetect format if possible */
		if (read(fd, &file_magic, sizeof(file_magic)) !=
		    sizeof(file_magic)) {
			goto peak_load_init_close;
		}

		/* rewind & pretend nothing happened */
		lseek(fd, 0, SEEK_SET);

		switch (file_magic) {
		case PCAP_MAGIC: {
			struct pcap_file_header hdr;

			if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
				goto peak_load_init_close;
			}

			ll = hdr.network;
			fmt = PCAP_FMT;

			break;
		}
		case PCAPNG_MAGIC: {
			struct pcapng_block_header hdr;

			if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
				goto peak_load_init_close;
			}

			/* skip over section header block */
			lseek(fd, hdr.length - sizeof(hdr), SEEK_CUR);

			/* change link type on the fly later */
			fmt = PCAPNG_FMT;
			ll = 0;

			break;
		}
		default:
			/* unknown magic means erf format */
			break;
		}
	}

	self = zalloc(sizeof(*self));
	if (!self) {
		goto peak_load_init_close;
	}

	self->fmt = fmt;
	self->fd = fd;
	self->ll = ll;

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
