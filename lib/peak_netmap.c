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

#ifdef __FreeBSD__

#include <peak.h>

#define NETMAP_DEBUG

/* debug support */
#ifdef NETMAP_DEBUG
#define D(format, ...)					\
	fprintf(stderr, "%s [%d] " format "\n",		\
	__FUNCTION__, __LINE__, ##__VA_ARGS__)
#else /* !NETMAP_DEBUG */
#define D(format, ...)	do {} while(0)
#endif /* NETMAP_DEBUG */

#include <errno.h>
#include <signal.h>	/* signal */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
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

#define NETMAP_MAX	64

/* Allow building on unmodified FreeBSD: */
#if NETMAP_API < 11
/* Use stubs. */
#else

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

struct _peak_netmap {
	struct netmap_ring *ring;
	unsigned int reserved;
	unsigned int i;
	struct peak_netmap data;
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

static inline struct _peak_netmap *
netpkt_from_user(struct peak_netmap *x)
{
	return (((struct _peak_netmap *)((x) + 1)) - 1);
}

static inline void
netpkt_put(struct _peak_netmap *x)
{
	prealloc_put(&self->pkt_pool, x);
}

static inline struct _peak_netmap *
netpkt_get()
{
	return prealloc_get(&self->pkt_pool);
}

static inline void *
netpkt_to_user(struct _peak_netmap *x)
{
	return &(x)->data;
}

/* Manage underlying pool resources */
static inline void
allocate_pool()
{
	bzero(self, sizeof(*self));

	prealloc_init(&self->pkt_pool, NETMAP_MAX,
		      sizeof(struct _peak_netmap));
	self->lastdev = 0;
}

static inline void
free_pool()
{
	prealloc_exit(&self->pkt_pool);
}

static int
_peak_netmap_init(struct peak_netmap_dev *me)
{
	uint16_t ringid = 0;
	int err;
	struct nmreq req;

	me->fd = open("/dev/netmap", O_RDWR);
	if (me->fd < 0) {
		D("Unable to open /dev/netmap");
		return (-1);
	}

	/* Put the interface into netmap mode: */
	bzero(&req, sizeof(req));
	req.nr_version = NETMAP_API;
	req.nr_ringid = 0;
	req.nr_flags = NR_REG_ALL_NIC;
	strlcpy(req.nr_name, me->ifname, sizeof(req.nr_name));
	err = ioctl(me->fd, NIOCREGIF, &req);
	if (err) {
		D("Unable to register %s", me->ifname);
		goto error;
	}
	me->memsize = req.nr_memsize;
	D("memsize is %d MB", me->memsize >> 20);

	/* Map associated memory: */
	if (me->mem == NULL) {
		me->mem = mmap(0, me->memsize, PROT_WRITE | PROT_READ, MAP_SHARED, me->fd, 0);
		if (me->mem == MAP_FAILED) {
			D("Unable to mmap");
			me->mem = NULL;
			goto error;
		}
	}


	/* Set the operating mode. */
	if (ringid != NETMAP_SW_RING) {
		const unsigned int if_flags = IFF_UP | IFF_PPROMISC;
		struct ifreq ifr;

		bzero(&ifr, sizeof(ifr));
		strlcpy(ifr.ifr_name, me->ifname, sizeof(ifr.ifr_name));

		if (ioctl(me->fd, SIOCGIFFLAGS, &ifr)) {
			D("ioctl error on SIOCGIFFLAGS");
			goto error;
		}

		ifr.ifr_flagshigh |= if_flags >> 16;
		ifr.ifr_flags |= if_flags & 0xffff;

		if (ioctl(me->fd, SIOCSIFFLAGS, &ifr)) {
			D("ioctl error on SIOCSIFFLAGS");
			goto error;
		}

		bzero(&ifr, sizeof(ifr));
		strlcpy(ifr.ifr_name, me->ifname, sizeof(ifr.ifr_name));

		if (ioctl(me->fd, SIOCGIFCAP, &ifr)) {
			D("ioctl error on SIOCGIFCAP");
			goto error;
		}

		ifr.ifr_reqcap = ifr.ifr_curcap;
		ifr.ifr_reqcap &= ~(IFCAP_HWCSUM | IFCAP_TSO | IFCAP_TOE);

		if (ioctl(me->fd, SIOCSIFCAP, &ifr)) {
			D("ioctl error on SIOCSIFCAP");
			goto error;
		}
	}

	me->nifp = NETMAP_IF(me->mem, req.nr_offset);
	me->queueid = ringid;

	switch (req.nr_flags) {
	case NR_REG_DEFAULT:
		abort();
	case NR_REG_ALL_NIC: /* our only supported case for now */
		me->begin = 0;
		me->end = req.nr_rx_rings; // XXX max of the two
		me->tx = NETMAP_TXRING(me->nifp, 0);
		me->rx = NETMAP_RXRING(me->nifp, 0);
		break;
	case NR_REG_SW: /* if (ringid & NETMAP_SW_RING) - ? */
		me->begin = req.nr_rx_rings;
		me->end = me->begin + 1;
		me->tx = NETMAP_TXRING(me->nifp, req.nr_tx_rings);
		me->rx = NETMAP_RXRING(me->nifp, req.nr_rx_rings);
		break;
	case NR_REG_NIC_SW: /* ? */
		break;
	case NR_REG_ONE_NIC: /* if (ringid & NETMAP_HW_RING) - ? */
		D("XXX check multiple threads");
		me->begin = ringid & NETMAP_RING_MASK;
		me->end = me->begin + 1;
		me->tx = NETMAP_TXRING(me->nifp, me->begin);
		me->rx = NETMAP_RXRING(me->nifp, me->begin);
		break;
	case NR_REG_PIPE_MASTER:
	case NR_REG_PIPE_SLAVE:
		break;
	default: /* broken */
		abort();
	};
	return (0);
error:
	close(me->fd);
	return -1;
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

static struct peak_netmap *
_peak_netmap_claim(const unsigned int want_sw)
{
	struct _peak_netmap *packet;
	struct peak_netmap_dev *dev;
	struct netmap_ring *ring;
	unsigned int j, rx;

	for (j = 0; j < self->lastdev; ++j) {
		dev = &self->dev[j];

		NR_FOREACH_RX(ring, rx, dev, want_sw) {
			unsigned int i, idx;

			if (nm_ring_empty(ring)) {
				continue;
			}

			packet = netpkt_get();
			if (!packet) {
				alert("netmap packet pool empty\n");
				return (NULL);
			}

			bzero(packet, sizeof(*packet));

			i = ring->cur;
			idx = ring->slot[i].buf_idx;
			if (idx < 2) {
				panic("%s bugus RX index %d at offset %d\n",
				    dev->nifp->ni_name, idx, i);
			}

			/* volatile internals */
			packet->ring = ring;
			packet->i = i;

			/* external stuff */
			packet->data.buf = NETMAP_BUF(ring, idx);
			packet->data.len = ring->slot[i].len;
			packet->data.ll = LINKTYPE_ETHERNET;
			packet->data.ts.tv_usec = ring->ts.tv_usec;
			packet->data.ts.tv_sec = ring->ts.tv_sec;
			packet->data.ifname = dev->ifname;

			return (netpkt_to_user(packet));
		}
	}

	return (NULL);
}

struct peak_netmap *
peak_netmap_claim(int timeout, const unsigned int want_sw)
{
	struct peak_netmap_dev *dev;
	struct peak_netmap *packet;
	struct pollfd *fd;
	unsigned int i;
	int ret;

	/*
	 * Look for packets prior to polling to avoid
	 * the system call overhead when not needed.
	 */

	packet = _peak_netmap_claim(want_sw);
	if (packet) {
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
			/*
			 * XXX This is quite naive: We know the fd,
			 * but go through all interfaces anyway.
			 */
			return (_peak_netmap_claim(want_sw));
		}
	}

	return (NULL);
}

unsigned int
peak_netmap_forward(struct peak_netmap *u_packet)
{
	struct _peak_netmap *packet = netpkt_from_user(u_packet);
	struct netmap_ring *source = packet->ring;
	struct netmap_slot *slot = &source->slot[packet->i];

	/* set the forward bit */
	slot->flags |= NS_FORWARD;

	/* drop kernel reference */
	source->head = source->cur = nm_ring_next(source, source->cur);

	/* drop userland reference */
	netpkt_put(packet);

	return (0);
}

unsigned int
peak_netmap_drop(struct peak_netmap *u_packet)
{
	struct _peak_netmap *packet = netpkt_from_user(u_packet);
	struct netmap_ring *source = packet->ring;
	struct netmap_slot *slot = &source->slot[packet->i];

	/* clear the forward bit */
	slot->flags &= ~NS_FORWARD;

	/* drop kernel reference */
	source->head = source->cur = nm_ring_next(source, source->cur);

	/* drop userland reference */
	netpkt_put(packet);

	return (0);
}

unsigned int
peak_netmap_divert(struct peak_netmap *u_packet, const char *ifname)
{
	/* assume the user is sane enough to not pass NULL */
	struct _peak_netmap *packet = netpkt_from_user(u_packet);
	struct netmap_ring *source = packet->ring;
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

		rs = &source->slot[packet->i];
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

		netpkt_put(packet);

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
		alert("netmap interface %s already attached\n");
		return (1);
	}

	if (self->lastdev >= sizeof(self->dev)/sizeof(self->dev[0])) {
		netmap_unlock();
		alert("netmap interface capacity reached\n");
		return (1);
	}

	devno = self->lastdev++; /* allocate it */

	bzero(&self->dev[devno], sizeof(self->dev[devno]));
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

#endif
#endif /* __FreeBSD__ */
