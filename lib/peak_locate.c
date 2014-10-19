/*
 * Copyright (c) 2012-2014 Franco Fichtner <franco@packetwerk.com>
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

#define LOCATE_DEFAULT	"/usr/local/var/peak/locate.bin"

struct peak_locates {
	struct peak_locate *mem;
	size_t count;
};

const char *
peak_locate_me(struct peak_locates *self, const struct netaddr *me)
{
	struct peak_locate *elm;
	struct peak_locate ref;
	const char *ret = "XX";

	if (self) {
		/*
		 * We know min and max match, but locatecmp
		 * doesn't know about it, so pass it on.
		 */
		ref.min = *me;
		ref.max = *me;

		elm = bsearch(&ref, self->mem, self->count,
		    sizeof(*self->mem), peak_locate_cmp);
		if (elm) {
			ret = elm->location;
		}
	}

	return (ret);
}

void
peak_locate_exit(struct peak_locates *self)
{
	if (self) {
		free(self->mem);
		free(self);
	}
}

struct peak_locates *
peak_locate_init(const char *file)
{
	struct peak_locate_hdr hdr;
	struct peak_locates *ret;
	struct peak_locate ref;
	unsigned int i;
	int fd;

	ret = malloc(sizeof(*ret));
	if (!ret) {
		alert("could not allocate memory\n");
		return (NULL);
	}

	ret->mem = NULL;
	ret->count = 0;

	if (!file) {
		file = LOCATE_DEFAULT;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		warning("could not open file `%s'\n", file);
		return (ret);
	}

	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		warning("file header not available\n");
		goto peak_locate_init_out;
	}

	if (hdr.magic != LOCATE_MAGIC) {
		warning("file magic mismatch\n");
		goto peak_locate_init_out;
	}

	if (hdr.revision != LOCATE_REVISION) {
		warning("file revision mismatch: got %u, want %u\n",
		    hdr.revision, LOCATE_REVISION);
		goto peak_locate_init_out;
	}

	if (!hdr.count) {
		goto peak_locate_init_out;
	}

	ret->mem = reallocarray(NULL, hdr.count, sizeof(*ret->mem));
	if (!ret->mem) {
		warning("memory allocation failed\n");
		goto peak_locate_init_out;
	}

	for (i = 0; i < hdr.count; ++i) {
		if (read(fd, &ref, sizeof(ref)) != sizeof(ref)) {
			warning("file not complete\n");
			free(ret->mem);
			ret->mem = NULL;
			goto peak_locate_init_out;
		}

		memcpy(&ret->mem[i], &ref, sizeof(ref));
	}

	ret->count = hdr.count;

peak_locate_init_out:

	close(fd);

	if (ret->count) {
		warning("successfully loaded %u locations\n", hdr.count);
	}

	return (ret);
}
