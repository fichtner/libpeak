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

static __thread struct peak_audit audit_thread;

static const char *audit_names[AUDIT_MAX] = {
	[AUDIT_PACKET_DROP_LINK] = "packet.drop.link",
	[AUDIT_PACKET_DROP_IPV4] = "packet.drop.ipv4",
	[AUDIT_PACKET_DROP_IPV6] = "packet.drop.ipv6",
	[AUDIT_PACKET_DROP_ICMP] = "packet.drop.icmp",
	[AUDIT_PACKET_DROP_TCP] = "packet.drop.tcp",
	[AUDIT_PACKET_DROP_UDP] = "packet.drop.udp",
};

const char *
peak_audit_name(const unsigned int field)
{
	const char *ret = NULL;

	if (likely(field < AUDIT_MAX)) {
		ret = audit_names[field];
	}

	return (ret);
}

void
peak_audit_set(const unsigned int field, const uint64_t value)
{
	if (likely(field < AUDIT_MAX)) {
		audit_thread.field[field] = value;
	}
}

void
peak_audit_add(const unsigned int field, const uint64_t value)
{
	if (likely(field < AUDIT_MAX)) {
		audit_thread.field[field] += value;
	}
}

void
peak_audit_inc(const unsigned int field)
{
	if (likely(field < AUDIT_MAX)) {
		++audit_thread.field[field];
	}
}

void
peak_audit_sync(struct peak_audit *export)
{
	unsigned int i;

	for (i = 0; i < AUDIT_MAX; ++i) {
		/* merge all fields by doing atomic ops */
		__sync_fetch_and_add(&export->field[i],
		    audit_thread.field[i]);
		/* wipe local fields for next round */
		peak_audit_set(i, 0);
	}
}
