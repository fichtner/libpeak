/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
 * Copyright (c) 2014 Alexey Saushev <alexey@packetwerk.com>
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

/*
 * Copyright (C) 2012 Luigi Rizzo. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <peak.h>

#include <errno.h>
#include <signal.h>	/* signal */
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>	/* PRI* macros */
#include <string.h>	/* strcmp */
#include <fcntl.h>	/* open */
#include <unistd.h>	/* close */
#include <ifaddrs.h>	/* getifaddrs */

#include <sys/mman.h>	/* PROT_* */
#include <sys/ioctl.h>	/* ioctl */
#include <sys/poll.h>
#include <sys/socket.h>	/* sockaddr.. */
#include <arpa/inet.h>	/* ntohs */
#include <sys/param.h>
#include <sys/sysctl.h>	/* sysctl */
#include <sys/time.h>	/* timersub */

#include <net/ethernet.h>
#include <net/if.h>	/* ifreq */
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <net/netmap.h>
#include <net/netmap_user.h>
#include <netinet/ip.h>

#include <machine/param.h>

#include <pthread_np.h> /* pthread w/ affinity */
#include <sys/cpuset.h> /* cpu_set */
#include <net/if_dl.h>  /* LLADDR */

const struct peak_transfers transfer_netmap = {
	.attach = peak_netmap_attach,
	.lock = peak_netmap_lock,
	.claim = peak_netmap_claim,
	.divert = peak_netmap_divert,
	.forward = peak_netmap_forward,
	.drop = peak_netmap_drop,
	.unlock = peak_netmap_unlock,
	.detach = peak_netmap_detach,
};

#define NETMAP_MAX	64

static pthread_mutex_t netmap_mutex = PTHREAD_MUTEX_INITIALIZER;

struct peak_netmap_dev {
	char ifname[IFNAMSIZ];
	int fd;
	char *mem;                      /* userspace mmap address */
	u_int memsize;
	u_int queueid;
	u_int begin, end;               /* first..last+1 rings to check */
	struct netmap_if *nifp;
	struct netmap_ring *tx, *rx;    /* shortcuts */
};

static struct peak_netmaps {
	unsigned int lastdev;
	struct peak_netmap_dev dev[NETMAP_MAX];

	prealloc_t pkt_pool;
	unsigned int busy;
	unsigned int reserved;
	struct pollfd fd[NETMAP_MAX];
} netmap_self = {.lastdev = 0};

static struct peak_netmaps *const self = &netmap_self;	/* global ref */

struct peak_netmap {
	struct netmap_ring *ring;
	unsigned int reserved;
	unsigned int i;
};

#define NR_FOREACH(mode, ring, idx, dev, want_sw)			\
	for ((idx) = (dev)->begin,					\
	    (ring) = NETMAP_##mode##RING((dev)->nifp, idx);		\
	    (idx) < (dev)->end + !!(want_sw); ++(idx),			\
	    (ring) = NETMAP_##mode##RING((dev)->nifp, idx))

#define NR_FOREACH_RX(r, i, d, s)	NR_FOREACH(RX, r, i, d, s)
#define NR_FOREACH_TX(r, i, d, s)	NR_FOREACH(TX, r, i, d, s)

static inline void
netmap_lock()
{
	pthread_mutex_lock(&netmap_mutex);
}

static inline void
netmap_unlock()
{
	pthread_mutex_unlock(&netmap_mutex);
}

static inline void
netpkt_put(struct peak_netmap *x)
{
	prealloc_put(&self->pkt_pool, x);
}

static inline struct peak_netmap *
netpkt_get()
{
	return prealloc_get(&self->pkt_pool);
}

/* Manage underlying pool resources */
static inline void
allocate_pool()
{
	memset(self, 0, sizeof(*self));

	prealloc_init(&self->pkt_pool, NETMAP_MAX,
	    sizeof(struct peak_netmap));
}

static inline void
free_pool()
{
	prealloc_exit(&self->pkt_pool);
}

static int
__peak_netmap_init(struct peak_netmap_dev *dev)
{
	const unsigned int if_flags = IFF_UP | IFF_PPROMISC;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, dev->ifname, sizeof(ifr.ifr_name));

	if (ioctl(dev->fd, SIOCGIFFLAGS, &ifr)) {
		error("ioctl error on SIOCGIFFLAGS");
		return (1);
	}

	ifr.ifr_flagshigh |= if_flags >> 16;
	ifr.ifr_flags |= if_flags & 0xffff;

	if (ioctl(dev->fd, SIOCSIFFLAGS, &ifr)) {
		error("ioctl error on SIOCSIFFLAGS");
		return (1);
	}

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, dev->ifname, sizeof(ifr.ifr_name));

	if (ioctl(dev->fd, SIOCGIFCAP, &ifr)) {
		error("ioctl error on SIOCGIFCAP");
		return (1);
	}

	ifr.ifr_reqcap = ifr.ifr_curcap;
	ifr.ifr_reqcap &= ~(IFCAP_HWCSUM | IFCAP_TSO | IFCAP_TOE);

	if (ioctl(dev->fd, SIOCSIFCAP, &ifr)) {
		error("ioctl error on SIOCSIFCAP");
		return (1);
	}

	return (0);
}
static int
_peak_netmap_init(struct peak_netmap_dev *dev)
{
	uint16_t ringid = 0;
	struct nmreq req;

	dev->fd = open("/dev/netmap", O_RDWR);
	if (dev->fd < 0) {
		error("unable to open /dev/netmap");
		return (1);
	}

	if (__peak_netmap_init(dev)) {
		goto error;
	}

	/* Put the interface into netmap mode: */
	memset(&req, 0, sizeof(req));
	req.nr_version = NETMAP_API;
	req.nr_ringid = ringid;
	req.nr_flags = NR_REG_ALL_NIC;
	strlcpy(req.nr_name, dev->ifname, sizeof(req.nr_name));

	if (ioctl(dev->fd, NIOCREGIF, &req)) {
		error("NIOCREGIF failed on %s", dev->ifname);
		goto error;
	}

	dev->memsize = req.nr_memsize;

	dev->mem = mmap(0, dev->memsize, PROT_WRITE | PROT_READ,
	    MAP_SHARED, dev->fd, 0);
	if (dev->mem == MAP_FAILED) {
		error("mmap() failed on %s\n", dev->ifname);
		goto error;
	}

	dev->nifp = NETMAP_IF(dev->mem, req.nr_offset);
	dev->queueid = ringid;

	/* setup for NR_REG_ALL_NIC */
	dev->begin = 0;
	dev->end = req.nr_rx_rings; // XXX max of the two
	dev->tx = NETMAP_TXRING(dev->nifp, 0);
	dev->rx = NETMAP_RXRING(dev->nifp, 0);

	return (0);

error:

	close(dev->fd);
	return (1);
}

static inline unsigned int
peak_netmap_find(const char *ifname)
{
	unsigned int i;

	if (!ifname) {
		return self->lastdev;
	}

	for (i = 0; i < self->lastdev; ++i) {
		if (!strcmp(ifname, self->dev[i].ifname)) {
			break;
		}
	}

	return (i);
}

static struct peak_transfer *
__peak_netmap_claim(struct peak_transfer *packet,
    struct peak_netmap_dev *dev, const unsigned int want_sw)
{
	struct peak_netmap *priv;
	struct netmap_ring *ring;
	unsigned int rx;

	NR_FOREACH_RX(ring, rx, dev, want_sw) {
		unsigned int i, idx;

		if (nm_ring_empty(ring)) {
			continue;
		}

		priv = netpkt_get();
		if (!priv) {
			alert("netmap packet pool empty\n");
			return (NULL);
		}

		memset(packet, 0, sizeof(*packet));
		memset(priv, 0, sizeof(*priv));

		i = ring->cur;
		idx = ring->slot[i].buf_idx;
		if (idx < 2) {
			panic("%s bugus RX index %d at offset %d\n",
			      dev->nifp->ni_name, idx, i);
		}

		/* volatile internals */
		priv->ring = ring;
		priv->i = i;

		/* external stuff */
		packet->buf = NETMAP_BUF(ring, idx);
		packet->len = ring->slot[i].len;
		packet->ll = LINKTYPE_ETHERNET;
		packet->ts.tv_usec = ring->ts.tv_usec;
		packet->ts.tv_sec = ring->ts.tv_sec;
		packet->ifname = dev->ifname;
		packet->private_data = priv;

		return (packet);
	}

	return (NULL);
};

static struct peak_transfer *
_peak_netmap_claim(struct peak_transfer *packet,
    const unsigned int want_sw)
{
	struct peak_transfer *ret = NULL;
	unsigned int j;

	for (j = 0; j < self->lastdev; ++j) {
		ret = __peak_netmap_claim(packet, &self->dev[j], want_sw);
		if (ret) {
			break;
		}
	}

	return (ret);
}

struct peak_transfer *
peak_netmap_claim(struct peak_transfer *packet, int timeout,
    const unsigned int want_sw)
{
	struct peak_netmap_dev *dev;
	struct pollfd *fd;
	unsigned int i;
	int ret;

	/*
	 * Look for packets prior to polling to avoid
	 * the system call overhead when not needed.
	 */

	if (_peak_netmap_claim(packet, want_sw)) {
		return (packet);
	}

	/*
	 * Call poll() to relax the CPU while adapters are idle.
	 * The caller is in charge of providing an appropriate
	 * timeout value (see poll(3)'s timeout parameter).
	 */

	for (i = 0; i < self->lastdev; ++i) {
		fd = &self->fd[i];

		fd->events = POLLIN | POLLOUT;
		fd->revents = 0;
	}

	ret = poll(self->fd, self->lastdev, timeout);
	if (ret <= 0) {
		/* error or timeout */
		return (NULL);
	}

	for (i = 0; i < self->lastdev; ++i) {
		dev = &self->dev[i];
		fd = &self->fd[i];

		if (fd->revents & POLLERR) {
			alert("error on %s, rxcur %d/%d", dev->ifname,
			    dev->rx->head, dev->rx->cur);
		}

		if (fd->revents & POLLIN) {
			return (__peak_netmap_claim(packet, dev, want_sw));
		}
	}

	return (NULL);
}

unsigned int
peak_netmap_forward(struct peak_transfer *packet)
{
	struct peak_netmap *priv = packet->private_data;
	struct netmap_ring *source = priv->ring;
	struct netmap_slot *slot = &source->slot[priv->i];

	/* set the forward bit */
	slot->flags |= NS_FORWARD;

	/* drop kernel reference */
	source->head = source->cur = nm_ring_next(source, source->cur);

	/* drop userland reference */
	netpkt_put(packet->private_data);

	/* scrub content */
	memset(packet, 0, sizeof(*packet));

	return (0);
}

unsigned int
peak_netmap_drop(struct peak_transfer *packet)
{
	struct peak_netmap *priv = packet->private_data;
	struct netmap_ring *source = priv->ring;
	struct netmap_slot *slot = &source->slot[priv->i];

	/* clear the forward bit */
	slot->flags &= ~NS_FORWARD;

	/* drop kernel reference */
	source->head = source->cur = nm_ring_next(source, source->cur);

	/* drop userland reference */
	netpkt_put(packet->private_data);

	/* scrub content */
	memset(packet, 0, sizeof(*packet));

	return (0);
}

unsigned int
peak_netmap_divert(struct peak_transfer *packet, const char *ifname)
{
	struct peak_netmap *priv = packet->private_data;
	struct netmap_ring *source = priv->ring;
	struct peak_netmap_dev *dev;
	struct netmap_ring *drain;
	unsigned int i, tx;

	if (!source) {
		/* packet is empty */
		return (0);
	}

	i = peak_netmap_find(ifname);
	if (i >= self->lastdev) {
		/* not found means error */
		return (1);
	}

	dev = &self->dev[i];

	NR_FOREACH_TX(drain, tx, dev, 0) {
		struct netmap_slot *rs, *ts;
		uint32_t pkt;

		if (nm_ring_empty(drain)) {
			continue;
		}

		i = drain->cur;

		rs = &source->slot[priv->i];
		ts = &drain->slot[i];

		pkt = ts->buf_idx;
		ts->buf_idx = rs->buf_idx;
		rs->buf_idx = pkt;

		ts->len = rs->len;

		/* report the buffer change */
		ts->flags |= NS_BUF_CHANGED;
		rs->flags |= NS_BUF_CHANGED;

		/* don't forward to host anymore */
		rs->flags &= ~NS_FORWARD;

		source->head = source->cur = nm_ring_next(source, source->cur);
		drain->head = drain->cur = nm_ring_next(drain, drain->cur);

		netpkt_put(packet->private_data);
		memset(packet, 0, sizeof(*packet));

		return (0);
	}

	/* could not release packet, try again */
	return (1);
}

static void
peak_netmap_configure(struct peak_netmap_dev *dev)
{
	struct netmap_ring *ring;
	unsigned int i;

	NR_FOREACH_RX(ring, i, dev, 1) {
		/* transparent mode: automatic forward via NS_FORWARD */
		ring->flags |= NR_FORWARD;
		/* timestamping: always update timestamps */
		ring->flags |= NR_TIMESTAMP;
	}
}

unsigned int
peak_netmap_attach(const char *ifname)
{
	unsigned int devno;

	netmap_lock();

	/* lazy init */
	if (self->lastdev == 0)
		allocate_pool();

	devno = peak_netmap_find(ifname);
	if (devno < self->lastdev) {
		netmap_unlock();
		alert("netmap interface %s already attached\n", ifname);
		return (1);
	}

	if (self->lastdev >= sizeof(self->dev)/sizeof(self->dev[0])) {
		netmap_unlock();
		alert("netmap interface capacity reached\n");
		return (1);
	}

	devno = self->lastdev++; /* allocate it */

	memset(&self->dev[devno], 0, sizeof(self->dev[devno]));
	strlcpy(self->dev[devno].ifname, ifname, sizeof(self->dev[devno].ifname));
	if (_peak_netmap_init(&self->dev[devno])) {
		self->lastdev--; /* deallocate it */
		netmap_unlock();
		alert("could not open netmap device %s\n", ifname);
		return (1);
	}

	peak_netmap_configure(&self->dev[devno]);
	self->fd[devno].fd = self->dev[devno].fd;

	netmap_unlock();

	return (0);
}

unsigned int
peak_netmap_detach(const char *ifname)
{
	unsigned int devno;

	netmap_lock();

	devno = peak_netmap_find(ifname);
	if (devno >= self->lastdev) {
		netmap_unlock();
		alert("netmap interface %s not attached\n", ifname);
		return (1);
	}

	if (self->dev[devno].mem) {
		munmap(self->dev[devno].mem, self->dev[devno].memsize);
	}

	close(self->dev[devno].fd);

	if (devno < self->lastdev - 1) {
		/* Replace the dead element with the last
		   element in the list to fill the hole in */
		self->dev[devno] = self->dev[self->lastdev - 1];
		self->fd[devno] = self->fd[self->lastdev - 1];
	};

	self->lastdev--;

	/* lazy exit */
	if (self->lastdev == 0)
		free_pool();

	netmap_unlock();

	return (0);
}

void
peak_netmap_lock(void)
{
	netmap_lock();
}

void
peak_netmap_unlock(void)
{
	netmap_unlock();
}
