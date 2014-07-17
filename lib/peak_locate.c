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

static struct peak_locates {
	struct peak_locate *mem;
	size_t count;
} locate_self;

static struct peak_locates *const self = &locate_self;

const char *
peak_locate_me(const struct netaddr *me)
{
	struct peak_locate *elm;
	struct peak_locate ref;

	/*
	 * We know min and max match, but locatecmp
	 * doesn't know about it, so pass it on.
	 */
	ref.min = *me;
	ref.max = *me;

	elm = bsearch(&ref, self->mem, self->count,
	    sizeof(*self->mem), peak_locate_cmp);
	if (!elm) {
		return ("XX");
	}

	return (elm->location);
}

void
peak_locate_exit(void)
{
	free(self->mem);

	self->mem = NULL;
	self->count = 0;
}

void
peak_locate_init(const char *file)
{
	struct peak_locate_hdr hdr;
	unsigned int inserted = 0;
	struct peak_locate *mem;
	struct peak_locate ref;
	unsigned int i;
	size_t count;
	int fd;

	if (!file) {
		/* reset the location database */
		peak_locate_exit();
		return;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		alert("file failed to open (skipped)\n");
		return;
	}

	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		alert("file header not available (skipped)\n");
		goto peak_locate_init_out;
	}

	if (hdr.magic != LOCATE_MAGIC) {
		alert("file magic mismatch (skipped)\n");
		goto peak_locate_init_out;
	}

	if (hdr.revision != LOCATE_REVISION) {
		alert("file revision mismatch: got %u, want %u "
		    "(skipped)\n", hdr.revision, LOCATE_REVISION);
		goto peak_locate_init_out;
	}

	count = hdr.count;
	if (!count) {
		goto peak_locate_init_out;
	}

	mem = reallocarray(NULL, count, sizeof(*mem));
	if (!mem) {
		alert("memory allocation failed\n");
		goto peak_locate_init_out;
	}

	for (i = 0; i < count; ++i) {
		if (read(fd, &ref, sizeof(ref)) != sizeof(ref)) {
			alert("file not complete (skipped)\n");
			free(mem);
			goto peak_locate_init_out;
		}

		memcpy(&mem[i], &ref, sizeof(ref));
		++inserted;
	}

	/* swap old and new values */
	peak_locate_exit();
	self->count = count;
	self->mem = mem;

peak_locate_init_out:

	close(fd);

	if (inserted) {
		alert("successfully loaded %u locations\n", inserted);
	}
}
