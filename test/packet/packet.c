/*
 * Copyright (c) 2014 Diego Assencio <diego@packetwerk.com>
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

#if defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#else /* !__OpenBSD__ && !__NetBSD__ */
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#endif /* __OpenBSD__ || __NetBSD__ */
#include <assert.h>

output_init();

/* source and destination IPv4 addresses: 1.2.3.4 / 5.6.7.8 */
static const uint32_t src_ip4 = 0x01020304;
static const uint32_t dst_ip4 = 0x05060708;

/* source and destination IPv6 addresses: fe80::1 / fe80::2 */
static uint8_t src_ip6[16] = { 0xfe,0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0x1 };
static uint8_t dst_ip6[16] = { 0xfe,0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0x2 };

static struct netaddr net_saddr4;
static struct netaddr net_daddr4;
static struct netaddr net_saddr6;
static struct netaddr net_daddr6;

/* source and destination ports for transport protocols (TCP/UDP) */
static uint16_t sport = 12345;
static uint16_t dport = 54321;

static void
test_icmp(const char* file)
{
	struct peak_load *trace = peak_load_init(file);
	struct peak_packet packet_info;
	unsigned int packet_len = 0;

	assert(trace);

	packet_len = peak_load_packet(trace);

	assert(!peak_packet_parse(&packet_info, trace->buf, packet_len,
	    LINKTYPE_ETHERNET));

	/* layer 1 */
	assert(packet_info.link_type == LINKTYPE_ETHERNET);

	/* layer 2 */
	assert(packet_info.mac_len == packet_len);
	assert(packet_info.mac_type == ETHERTYPE_IP);
	packet_len -= sizeof(struct ether_header);

	/* layer 3 */
	assert(!netcmp(&packet_info.net_saddr, &net_saddr4));
	assert(!netcmp(&packet_info.net_daddr, &net_daddr4));
	assert(packet_info.net_family == AF_INET);
	assert(packet_info.net_len == packet_len);
	packet_len -= packet_info.net_hlen;

	/* layer 4 */
	assert(packet_info.flow_len == packet_len);
	packet_len -= packet_info.flow_hlen;

	/* layer 7 */
	assert(packet_info.app_len = packet_len);

	peak_load_exit(trace);
}

/* this test works for both TCP and UDP over IPv4 */
static void
test_transport_ip4(const char* file)
{
	struct peak_load *trace = peak_load_init(file);
	struct peak_packet packet_info;
	unsigned int packet_len = 0;

	assert(trace);

	packet_len = peak_load_packet(trace);

	assert(!peak_packet_parse(&packet_info, trace->buf, packet_len,
	    LINKTYPE_ETHERNET));

	/* layer 1 */
	assert(packet_info.link_type == LINKTYPE_ETHERNET);

	/* layer 2 */
	assert(packet_info.mac_len == packet_len);
	assert(packet_info.mac_type == ETHERTYPE_IP);
	packet_len -= sizeof(struct ether_header);

	/* layer 3 */
	assert(!netcmp(&packet_info.net_saddr, &net_saddr4));
	assert(!netcmp(&packet_info.net_daddr, &net_daddr4));
	assert(packet_info.net_family == AF_INET);
	assert(packet_info.net_len == packet_len);
	packet_len -= packet_info.net_hlen;

	/* layer 4 */
	assert(packet_info.flow_len == packet_len);
	assert(packet_info.flow_sport == sport);
	assert(packet_info.flow_dport == dport);
	packet_len -= packet_info.flow_hlen;

	/* layer 7 */
	assert(packet_info.app_len = packet_len);

	peak_load_exit(trace);
}

/* this test works for both TCP and UDP over IPv6 */
static void
test_transport_ip6(const char* file)
{
	struct peak_load *trace = peak_load_init(file);
	struct peak_packet packet_info;
	unsigned int packet_len = 0;

	assert(trace);

	packet_len = peak_load_packet(trace);

	assert(!peak_packet_parse(&packet_info, trace->buf, packet_len,
	    LINKTYPE_ETHERNET));

	/* layer 1 */
	assert(packet_info.link_type == LINKTYPE_ETHERNET);

	/* layer 2 */
	assert(packet_info.mac_len == packet_len);
	assert(packet_info.mac_type == ETHERTYPE_IPV6);
	packet_len -= sizeof(struct ether_header);

	/* layer 3 */
	assert(!netcmp(&packet_info.net_saddr, &net_saddr6));
	assert(!netcmp(&packet_info.net_daddr, &net_daddr6));
	assert(packet_info.net_family == AF_INET6);
	assert(packet_info.net_len == packet_len);
	packet_len -= packet_info.net_hlen;

	/* layer 4 */
	assert(packet_info.flow_len == packet_len);
	assert(packet_info.flow_sport == sport);
	assert(packet_info.flow_dport == dport);
	packet_len -= packet_info.flow_hlen;

	/* layer 7 */
	assert(packet_info.app_len = packet_len);

	peak_load_exit(trace);
}

int
main(void)
{
	static const char *pcap_files[] = {
		"../../sample/icmp.pcap",
		"../../sample/tcp.pcap",
		"../../sample/udp.pcap",
		"../../sample/tcp6.pcap",
		"../../sample/udp6.pcap",
	};

	pout("peak packet test suite... ");

	/* make sure this is always aligned to 8 bytes */
	assert(!(sizeof(struct peak_packet) % sizeof(uint64_t)));

	memset(&net_saddr4, 0, sizeof(net_saddr4));
	memset(&net_daddr4, 0, sizeof(net_daddr4));
	memset(&net_saddr6, 0, sizeof(net_saddr6));
	memset(&net_daddr6, 0, sizeof(net_daddr6));

	netaddr4(&net_saddr4, be32dec(&src_ip4));
	netaddr4(&net_daddr4, be32dec(&dst_ip4));
	netaddr6(&net_saddr6, src_ip6);
	netaddr6(&net_daddr6, dst_ip6);

	test_icmp(pcap_files[0]);
	test_transport_ip4(pcap_files[1]); /* TCP over IPv4 */
	test_transport_ip4(pcap_files[2]); /* UDP over IPv4 */
	test_transport_ip6(pcap_files[3]); /* TCP over IPv6 */
	test_transport_ip6(pcap_files[4]); /* UDP over IPv6 */

	pout("ok\n");

	return (0);
}
