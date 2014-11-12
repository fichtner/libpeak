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

#include <peak.h>
#include <assert.h>

static void
test_audit(void)
{
	struct peak_audit data;
	unsigned int i;

	/* data is global and only increased, so clear it */
	memset(&data, 0, sizeof(data));
	peak_audit_sync(&data);

	for (i = 0; i < AUDIT_MAX; ++i) {
		/* all fields must have a name string */
		assert(peak_audit_name(i));
		/* all fields must be empty on startup */
		assert(!data.field[i]);
		/* all fields are cleared by previous sync() */
		assert(!peak_audit_get(i));
	}

	/* internal field addition and increase */
	peak_audit_add(0, 41);
	assert(peak_audit_get(0) == 41);
	/* get() does not clear the state */
	assert(peak_audit_get(0) == 41);
	peak_audit_sync(&data);
	assert(data.field[0] == 41);
	peak_audit_inc(0);
	assert(peak_audit_get(0) == 1);
	peak_audit_sync(&data);
	assert(data.field[0] == 42);
	assert(peak_audit_get(0) == 0);

	/* internal fields are cleared so result is the same */
	peak_audit_sync(&data);
	assert(data.field[0] == 42);

	/* set it to something else and try again */
	peak_audit_set(0, 7);
	peak_audit_sync(&data);
	assert(data.field[0] == 49);

	/* clear aggregate and resync, causing eternal winter to unfold */
	memset(&data, 0, sizeof(data));
	peak_audit_sync(&data);
	assert(!data.field[0]);
}

int
main(void)
{
	pout("peak audit test suite... ");

	test_audit();

	pout("ok\n");

	return (0);
}
