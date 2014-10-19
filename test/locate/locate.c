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

#include <peak.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

output_init();

static void
test_mapping(void)
{
	unsigned int i, number;
	const char *name;

	for (i = LOCATE_UNKNOWN; i < LOCATE_MAX; ++i) {
		/* all protocols have names... */
		name = peak_locate_name(i);
		assert(name);
		/* ...and a unique number */
		number = peak_locate_number(name);
		assert(i == number);
	}
}

static void
test_locate(void)
{
	struct peak_locates *db;
	struct netaddr test_ip;

	db = peak_locate_init("../../config/does.not.exist");
	assert(db);

	netaddr4(&test_ip, inet_addr("88.74.143.194"));
	assert(!peak_locate_get(db, &test_ip));

	peak_locate_exit(NULL);
	peak_locate_exit(db);

	db = peak_locate_init("../../config/locate.csv.bin");
	assert(db);

	netaddr4(&test_ip, inet_addr("88.74.143.194"));
	assert(peak_locate_get(db, &test_ip) == LOCATE_DE);

	netaddr4(&test_ip, inet_addr("0.116.0.0"));
	assert(peak_locate_get(db, &test_ip) == LOCATE_AT);

	netaddr4(&test_ip, inet_addr("0.115.255.255"));
	assert(peak_locate_get(db, &test_ip) == LOCATE_XX);

	netaddr4(&test_ip, inet_addr("0.119.255.255"));
	assert(peak_locate_get(db, &test_ip) == LOCATE_AT);

	netaddr4(&test_ip, inet_addr("0.120.0.0"));
	assert(peak_locate_get(db, &test_ip) == LOCATE_XX);

	inet_pton(AF_INET6, "2001:618::", &test_ip);
	assert(peak_locate_get(db, &test_ip) == LOCATE_CH);

	inet_pton(AF_INET6, "2001:617:ffff:ffff:ffff:ffff:ffff:ffff",
	    &test_ip);
	assert(peak_locate_get(db, &test_ip) == LOCATE_XX);

	inet_pton(AF_INET6, "2001:618:ffff:ffff:ffff:ffff:ffff:ffff",
	    &test_ip);
	assert(peak_locate_get(db, &test_ip) == LOCATE_CH);

	inet_pton(AF_INET6, "2001:619::", &test_ip);
	assert(peak_locate_get(db, &test_ip) == LOCATE_XX);

	peak_locate_exit(db);
}

int
main(void)
{
	pout("peak locate test suite... ");

	test_mapping();
	test_locate();

	pout("ok\n");

	return (0);
}
