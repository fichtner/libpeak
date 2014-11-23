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

#if !defined(NETMAP_API) || NETMAP_API < 11

const struct peak_transfers transfer_netmap = {
	.attach = peak_transfer_attach,
	.master = peak_transfer_master,
	.slave = peak_transfer_slave,
	.lock = peak_transfer_lock,
	.recv = peak_transfer_recv,
	.send = peak_transfer_send,
	.forward = peak_transfer_forward,
	.unlock = peak_transfer_unlock,
	.detach = peak_transfer_detach,
};

#else

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
	.master = peak_netmap_master,
	.slave = peak_netmap_slave,
	.lock = peak_netmap_lock,
	.recv = peak_netmap_recv,
	.send = peak_netmap_send,
	.forward = peak_netmap_forward,
	.unlock = peak_netmap_unlock,
	.detach = peak_netmap_detach,
};

#define NETMAP_MAX	64

static pthread_mutex_t netmap_mutex = PTHREAD_MUTEX_INITIALIZER;

struct peak_netmap_dev {
	char ifname[IFNAMSIZ];
	struct netmap_if *nifp;
	void *mem;			/* userspace mmap address */
	int fd;
	unsigned int type;
	unsigned int memsize;
	unsigned int begin;		/* first ring to check */
	unsigned int end;		/* last+1 ring to check */
};

struct peak_netmap {
	struct netmap_ring *ring;
	unsigned int used;
	unsigned int i;
};

static struct peak_netmaps {
	unsigned int count;
	unsigned int last;
	struct peak_netmap private_data;
	struct peak_netmap_dev dev[NETMAP_MAX];
	struct pollfd fd[NETMAP_MAX];
} netmap_self;

static struct peak_netmaps *const self = &netmap_self;	/* global ref */

#define NR_FOREACH_WIRE(mode, ring, idx, dev)				\
	for ((idx) = (dev)->begin,					\
	    (ring) = NETMAP_##mode##RING((dev)->nifp, idx);		\
	    (idx) < (dev)->end; ++(idx),				\
	    (ring) = NETMAP_##mode##RING((dev)->nifp, idx))

#define NR_FOREACH_HOST(mode, ring, idx, dev)				\
	for ((idx) = (dev)->end,					\
	    (ring) = NETMAP_##mode##RING((dev)->nifp, idx);		\
	    (idx) <= (dev)->end; ++(idx),				\
	    (ring) = NETMAP_##mode##RING((dev)->nifp, idx))

#define NETMAP_LOCK() do {						\
	pthread_mutex_lock(&netmap_mutex);				\
} while (0)

#define NETMAP_UNLOCK() do {						\
	pthread_mutex_unlock(&netmap_mutex);				\
} while (0)

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
	struct nmreq req;

	memset(&req, 0, sizeof(req));

	dev->fd = open("/dev/netmap", O_RDWR);
	if (dev->fd < 0) {
		error("unable to open /dev/netmap\n");
		return (1);
	}

	if (dev->type == NR_REG_ALL_NIC && __peak_netmap_init(dev)) {
		goto _peak_netmap_init_error;
	}

	/* Put the interface into netmap mode: */
	req.nr_version = NETMAP_API;
	req.nr_flags = dev->type;

	switch (dev->type) {
	case NR_REG_PIPE_MASTER:
	case NR_REG_PIPE_SLAVE:
		/* hardcoded name, meh */
		strlcpy(req.nr_name, "vale", sizeof(req.nr_name));
		/* must extract ring id */
		req.nr_ringid = atoi(dev->ifname);
		if (req.nr_ringid >= NETMAP_RING_MASK) {
			goto _peak_netmap_init_error;
		}
		break;
	default:
		strlcpy(req.nr_name, dev->ifname, sizeof(req.nr_name));
		req.nr_ringid = 0;
		break;
	}

	if (ioctl(dev->fd, NIOCREGIF, &req)) {
		error("NIOCREGIF failed on %s\n", dev->ifname);
		goto _peak_netmap_init_error;
	}

	dev->memsize = req.nr_memsize;

	dev->mem = mmap(0, dev->memsize, PROT_WRITE | PROT_READ,
	    MAP_SHARED, dev->fd, 0);
	if (dev->mem == MAP_FAILED) {
		error("mmap() failed on %s\n", dev->ifname);
		goto _peak_netmap_init_error;
	}

	dev->nifp = NETMAP_IF(dev->mem, req.nr_offset);

	/* setup for NR_REG_ALL_NIC */
	dev->begin = 0;
	dev->end = req.nr_rx_rings; // XXX max of the two

	return (0);

_peak_netmap_init_error:

	close(dev->fd);
	return (1);
}

static inline unsigned int
peak_netmap_find(const char *ifname, const unsigned int ignore)
{
	unsigned int i;

	if (!ifname) {
		return self->count;
	}

	for (i = 0; i < self->count; ++i) {
		if (ignore != NR_REG_ALL_NIC &&
		    ignore == self->dev[i].type) {
			/* must ignore pipe then */
			continue;
		}
		if (!strcmp(ifname, self->dev[i].ifname)) {
			break;
		}
	}

	return (i);
}

static struct peak_transfer *
___peak_netmap_recv(struct peak_transfer *packet,
    struct peak_netmap_dev *dev, struct netmap_ring *ring,
    const unsigned int rx)
{
	struct peak_netmap *priv = &self->private_data;
	unsigned int i, idx;

	if (nm_ring_empty(ring)) {
		return (NULL);
	}

	memset(packet, 0, sizeof(*packet));

	i = ring->cur;
	idx = ring->slot[i].buf_idx;
	if (unlikely(idx < 2)) {
		/* XXX this can probably be zapped */
		panic("%s bogus RX index %d at offset %d\n",
		    dev->nifp->ni_name, idx, i);
	}

	/* volatile internals */
	priv->ring = ring;
	priv->used = 1;
	priv->i = i;

	/* external stuff */
	packet->ts.tv_usec = ring->ts.tv_usec;
	packet->ts.tv_sec = ring->ts.tv_sec;
	packet->buf = NETMAP_BUF(ring, idx);
	packet->len = ring->slot[i].len;
	packet->ll = LINKTYPE_ETHERNET;
	packet->ifname = dev->ifname;
	packet->private_data = priv;

	/* packet origin info */
	packet->id = idx;

	switch (dev->type) {
	case NR_REG_PIPE_SLAVE:
		packet->type = NETMAP_PIPE;
		break;
	default:
		packet->type = rx != dev->end ?
		    NETMAP_WIRE : NETMAP_HOST;
		break;
	}

	return (packet);
}

static struct peak_transfer *
__peak_netmap_recv(struct peak_transfer *packet,
    struct peak_netmap_dev *dev, const unsigned int mode)
{
	struct netmap_ring *ring;
	unsigned int rx;

	if (dev->type == NR_REG_PIPE_MASTER) {
		/* can't read from master pipe */
		return (NULL);
	}

	switch (mode) {
	case NETMAP_DFLT:
	case NETMAP_HOST:
		if (dev->type != NR_REG_PIPE_SLAVE) {
			/*
			 * Host not requested or nonexisting
			 * ring on slave pipe (I think).
			 */
			NR_FOREACH_HOST(RX, ring, rx, dev) {
				if (___peak_netmap_recv(packet, dev,
				    ring, rx)) {
					return (packet);
				}
			}
			if (mode == NETMAP_HOST) {
				break;
			}
		}
		/* FALLTHROUGH */
	default:
		NR_FOREACH_WIRE(RX, ring, rx, dev) {
			if (___peak_netmap_recv(packet, dev,
			    ring, rx)) {
				return (packet);
			}
		}
		break;
	}

	return (NULL);
}

static struct peak_transfer *
_peak_netmap_recv(struct peak_transfer *packet,
    const unsigned int mode)
{
	unsigned int i, j = self->last + 1;
	struct peak_transfer *ret = NULL;

	for (i = 0; i < self->count; ++i) {
		/* reshuffle the index */
		j = (j + i) % self->count;
		ret = __peak_netmap_recv(packet, &self->dev[j], mode);
		if (ret) {
			/* set the last device */
			self->last = j;
			break;
		}
	}

	return (ret);
}

struct peak_transfer *
peak_netmap_recv(struct peak_transfer *packet, int timeout,
    const char *ifname, const unsigned int mode)
{
	struct peak_netmap *priv = &self->private_data;
	struct peak_netmap_dev *dev;
	struct pollfd *fd;
	unsigned int i;
	int ret;

	(void)ifname;

	if (unlikely(priv->used)) {
		alert("must release claimed packet first\n");
		return (NULL);
	}

	/*
	 * Look for packets prior to polling to avoid
	 * the system call overhead when not needed.
	 */

	if (_peak_netmap_recv(packet, mode)) {
		return (packet);
	}

	/*
	 * Call poll() to relax the CPU while adapters are idle.
	 * The caller is in charge of providing an appropriate
	 * timeout value (see poll(3)'s timeout parameter).
	 */

	for (i = 0; i < self->count; ++i) {
		fd = &self->fd[i];

		fd->events = POLLIN | POLLOUT;
		fd->revents = 0;
	}

	ret = poll(self->fd, self->count, timeout);
	if (ret <= 0) {
		/* error or timeout */
		return (NULL);
	}

	for (i = 0; i < self->count; ++i) {
		dev = &self->dev[i];
		fd = &self->fd[i];

		if (fd->revents & POLLERR) {
			alert("poll() error on %s\n", dev->ifname);
		}

		if (fd->revents & POLLIN) {
			return (__peak_netmap_recv(packet, dev, mode));
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

	/* scrub content */
	memset(packet, 0, sizeof(*packet));
	memset(priv, 0, sizeof(*priv));

	return (0);
}

static unsigned int
peak_netmap_drop(struct peak_transfer *packet)
{
	struct peak_netmap *priv = packet->private_data;
	struct netmap_ring *source = priv->ring;
	struct netmap_slot *slot = &source->slot[priv->i];

	/* clear the forward bit */
	slot->flags &= ~NS_FORWARD;

	/* drop kernel reference */
	source->head = source->cur = nm_ring_next(source, source->cur);

	/* scrub content */
	memset(packet, 0, sizeof(*packet));
	memset(priv, 0, sizeof(*priv));

	return (0);
}

static unsigned int
_peak_netmap_send(struct peak_transfer *packet, struct netmap_ring *ring)
{
	struct peak_netmap *priv = packet->private_data;
	struct netmap_ring *curr = priv->ring;
	struct netmap_slot *rs, *ts;
	unsigned int i, idx;

	if (nm_ring_empty(ring)) {
		return (1);
	}

	i = ring->cur;

	rs = &curr->slot[priv->i];
	ts = &ring->slot[i];

	/* zero-copy magic */
	idx = ts->buf_idx;
	ts->buf_idx = rs->buf_idx;
	rs->buf_idx = idx;
	ts->len = rs->len;

	/* report the buffer change */
	ts->flags |= NS_BUF_CHANGED;
	rs->flags |= NS_BUF_CHANGED;

	/* don't forward to host anymore */
	rs->flags &= ~NS_FORWARD;

	curr->head = curr->cur = nm_ring_next(curr, curr->cur);
	ring->head = ring->cur = nm_ring_next(ring, ring->cur);

	/* scrub content */
	memset(packet, 0, sizeof(*packet));
	memset(priv, 0, sizeof(*priv));

	return (0);
}

unsigned int
peak_netmap_send(struct peak_transfer *packet, const char *ifname,
    const unsigned int mode)
{
	struct peak_netmap *priv = packet->private_data;
	struct peak_netmap_dev *dev;
	struct netmap_ring *ring;
	unsigned int i, tx;

	if (!priv->used) {
		/* packet is empty */
		return (0);
	}

	if (!ifname) {
		peak_netmap_drop(packet);
	}

	i = peak_netmap_find(ifname, NR_REG_PIPE_SLAVE);
	if (i >= self->count) {
		/* not found means error */
		return (1);
	}

	dev = &self->dev[i];

	switch (mode) {
	case NETMAP_HOST:
		NR_FOREACH_HOST(TX, ring, tx, dev) {
			if (!_peak_netmap_send(packet, ring)) {
				return (0);
			}
		}
		break;
	default:
		NR_FOREACH_WIRE(TX, ring, tx, dev) {
			if (!_peak_netmap_send(packet, ring)) {
				return (0);
			}
		}
		break;
	}

	/* could not release packet, try again */
	return (1);
}

static void
peak_netmap_configure(struct peak_netmap_dev *dev)
{
	struct netmap_ring *ring;
	unsigned int i;

	NR_FOREACH_WIRE(RX, ring, i, dev) {
		/* transparent mode: automatic forward via NS_FORWARD */
		ring->flags |= NR_FORWARD;
		/* timestamping: always update timestamps */
		ring->flags |= NR_TIMESTAMP;
	}

	NR_FOREACH_HOST(RX, ring, i, dev) {
		ring->flags |= NR_FORWARD | NR_TIMESTAMP;
	}
}

static unsigned int
_peak_netmap_attach(const char *ifname, const unsigned int type)
{
	unsigned int ignore = type;
	unsigned int i;

	switch (type) {
	case NR_REG_PIPE_MASTER:
		ignore = NR_REG_PIPE_SLAVE;
		break;
	case NR_REG_PIPE_SLAVE:
		ignore = NR_REG_PIPE_MASTER;
		break;
	default:
		break;
	}

	i = peak_netmap_find(ifname, ignore);
	if (i < self->count) {
		alert("netmap interface %s already attached\n", ifname);
		return (1);
	}

	if (self->count >= lengthof(self->dev)) {
		alert("netmap interface capacity reached\n");
		return (1);
	}

	i = self->count;

	memset(&self->dev[i], 0, sizeof(self->dev[i]));
	strlcpy(self->dev[i].ifname, ifname, sizeof(self->dev[i].ifname));
	self->dev[i].type = type;
	if (_peak_netmap_init(&self->dev[i])) {
		alert("could not open netmap device %s\n", ifname);
		return (1);
	}

	peak_netmap_configure(&self->dev[i]);
	self->fd[i].fd = self->dev[i].fd;

	++self->count;

	return (0);
}

unsigned int
peak_netmap_master(const char *pipeid)
{
	return (_peak_netmap_attach(pipeid, NR_REG_PIPE_MASTER));
}

unsigned int
peak_netmap_slave(const char *pipeid)
{
	return (_peak_netmap_attach(pipeid, NR_REG_PIPE_SLAVE));
}

unsigned int
peak_netmap_attach(const char *ifname)
{
	return (_peak_netmap_attach(ifname, NR_REG_ALL_NIC));
}

unsigned int
peak_netmap_detach(const char *ifname)
{
	unsigned int i;

	i = peak_netmap_find(ifname, NR_REG_ALL_NIC);
	if (i >= self->count) {
		alert("netmap interface %s not attached\n", ifname);
		return (1);
	}

	if (self->dev[i].mem) {
		munmap(self->dev[i].mem, self->dev[i].memsize);
	}

	close(self->dev[i].fd);

	if (i < self->count - 1) {
		/*
		 * Replace the dead element with the last
		 * element in the list to fill the hole.
		 */
		self->dev[i] = self->dev[self->count - 1];
		self->fd[i] = self->fd[self->count - 1];
	};

	self->count--;

	return (0);
}

void
peak_netmap_lock(void)
{
	NETMAP_LOCK();
}

void
peak_netmap_unlock(void)
{
	NETMAP_UNLOCK();
}

#endif
