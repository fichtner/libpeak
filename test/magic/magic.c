/*
 * Copyright (c) 2014 Tobias Boertitz <tobias@packetwerk.com>
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
#include <assert.h>

output_init();

static void
test_mapping(void)
{
	unsigned int i, number;
	const char *name;

	for (i = MAGIC_UNKNOWN; i < MAGIC_MAX; ++i) {
		/* all file types have names... */
		name = peak_magic_name(i);
		assert(name);
		/* ...and a unique number */
		number = peak_magic_number(name);
		assert(i == number);
	}
}

static void
test_sample(const char *file, const unsigned int type, const char *name)
{
	struct peak_magic *magic;
	char buf[4096];
	size_t size;
	int fd;

	fd = open(file, O_RDONLY);
	assert(fd >= 0);

	size = read(fd, buf, sizeof(buf));
	assert(size > 0);

	magic = peak_magic_init();
	assert(magic);

	assert(type == peak_magic_get(magic, buf, size));

	assert(peak_magic_name(type));
	assert(!strcmp(name, peak_magic_name(type)));

	peak_magic_exit(magic);
	close(fd);
}

int
main(void)
{
	pout("peak magic test suite... ");

	test_mapping();
	test_sample("../../test/magic/hello.c",
	    MAGIC_C_SOURCE_TEXT, "text/x-c");
	test_sample("../../test/magic/hello.sh",
	    MAGIC_SHELLSCRIPT_TEXT_EXECUTABLE, "text/x-shellscript");
	test_sample("../../test/magic/hello.pdf",
	    MAGIC_PDF_DOCUMENT, "application/pdf");

	pout("ok\n");

	return (0);
}
