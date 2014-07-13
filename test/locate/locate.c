/*
 * Copyright (c) 2012-2013 Franco Fichtner <franco@packetwerk.com>
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
test_locate(void)
{
	struct netaddr test_ip;

	peak_locate_init("../../config/locate.csv.bin");

	netaddr4(&test_ip, inet_addr("88.74.143.194"));
	assert(!strcmp(peak_locate_me(&test_ip), "DE"));

	netaddr4(&test_ip, inet_addr("0.116.0.0"));
	assert(!strcmp(peak_locate_me(&test_ip), "AT"));

	netaddr4(&test_ip, inet_addr("0.115.255.255"));
	assert(!strcmp(peak_locate_me(&test_ip), "XX"));

	netaddr4(&test_ip, inet_addr("0.119.255.255"));
	assert(!strcmp(peak_locate_me(&test_ip), "AT"));

	netaddr4(&test_ip, inet_addr("0.120.0.0"));
	assert(!strcmp(peak_locate_me(&test_ip), "XX"));

	inet_pton(AF_INET6, "2001:618::", &test_ip);
	assert(!strcmp(peak_locate_me(&test_ip), "CH"));

	inet_pton(AF_INET6, "2001:617:ffff:ffff:ffff:ffff:ffff:ffff",
	    &test_ip);
	assert(!strcmp(peak_locate_me(&test_ip), "XX"));

	inet_pton(AF_INET6, "2001:618:ffff:ffff:ffff:ffff:ffff:ffff",
	    &test_ip);
	assert(!strcmp(peak_locate_me(&test_ip), "CH"));

	inet_pton(AF_INET6, "2001:619::", &test_ip);
	assert(!strcmp(peak_locate_me(&test_ip), "XX"));

	peak_locate_exit();
}

int
main(void)
{
	pout("peak locate test suite... ");

	test_locate();

	pout("ok\n");

	return (0);
}
