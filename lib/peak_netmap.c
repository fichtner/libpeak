/*
 * Copyright (c) 2013-2014 Franco Fichtner <franco@packetwerk.com>
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

#ifdef __linux__
#undef __unused	/* clashes with struct member in <sys/sysctl.h> */
#endif /* __linux__ */

#include "../contrib/libnetmap/nm_util.h"

#define NETMAP_MAX	64

#define NETMAP_LOCK() do {						\
	pthread_mutex_lock(&netmap_mutex);				\
} while (0)

#define NETMAP_UNLOCK() do {						\
	pthread_mutex_unlock(&netmap_mutex);				\
} while (0)

#define NETPKT_FROM_USER(x)	(((struct _peak_netmap *)((x) + 1)) - 1)
#define NETPKT_PUT(x)		prealloc_put(&self->pkt_pool, x)
#define NETPKT_GET(x)		prealloc_get(&self->pkt_pool)
#define NETPKT_TO_USER(x)	&(x)->data

#define NETMAP_PUT(x)		prealloc_put(&self->dev_pool, x)
#define NETMAP_COUNT()		prealloc_used(&self->dev_pool)
#define NETMAP_GET()		prealloc_get(&self->dev_pool)

static pthread_mutex_t netmap_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct peak_netmaps {
	prealloc_t pkt_pool;
	prealloc_t dev_pool;
	unsigned int busy;
	unsigned int reserved;
	struct my_ring *dev[NETMAP_MAX];
	struct pollfd fd[NETMAP_MAX];
} netmap_self;

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

static inline unsigned int
peak_netmap_find(const char *ifname)
{
	unsigned int i;

	if (!ifname) {
		return (NETMAP_COUNT());
	}

	for (i = 0; i < NETMAP_COUNT(); ++i) {
		if (!strcmp(ifname, self->dev[i]->ifname)) {
			break;
		}
	}

	return (i);
}

static struct peak_netmap *
_peak_netmap_claim(const unsigned int want_sw)
{
	struct _peak_netmap *packet;
	struct netmap_ring *ring;
	struct my_ring *dev;
	unsigned int j, rx;

	for (j = 0; j < NETMAP_COUNT(); ++j) {
		dev = self->dev[j];

		NR_FOREACH_RX(ring, rx, dev, want_sw) {
			unsigned int i, idx;

			if (!ring->avail) {
				continue;
			}

			packet = NETPKT_GET();
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

			return (NETPKT_TO_USER(packet));
		}
	}

	return (NULL);
}

struct peak_netmap *
peak_netmap_claim(int timeout, const unsigned int want_sw)
{
	struct peak_netmap *packet;
	struct my_ring *dev;
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

	for (i = 0; i < NETMAP_COUNT(); ++i) {
		fd = &self->fd[i];

		fd->events = POLLIN;
		fd->revents = 0;
	}

	ret = poll(self->fd, NETMAP_COUNT(), timeout);
	if (ret <= 0) {
		/* error or timeout */
		return (NULL);
	}

	for (i = 0; i < NETMAP_COUNT(); ++i) {
		dev = self->dev[i];
		fd = &self->fd[i];

		if (fd->revents & POLLERR) {
			alert("error on %s, rxcur %d@%d", dev->ifname,
			    dev->rx->avail, dev->rx->cur);
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
	struct _peak_netmap *packet = NETPKT_FROM_USER(u_packet);
	struct netmap_ring *source = packet->ring;
	struct netmap_slot *slot = &source->slot[packet->i];

	/* set the forward bit */
	slot->flags |= NS_FORWARD;

	/* drop kernel reference */
	source->cur = NETMAP_RING_NEXT(source, packet->i);
	--source->avail;

	/* drop userland reference */
	NETPKT_PUT(packet);

	return (0);
}

unsigned int
peak_netmap_drop(struct peak_netmap *u_packet)
{
	struct _peak_netmap *packet = NETPKT_FROM_USER(u_packet);
	struct netmap_ring *source = packet->ring;
	struct netmap_slot *slot = &source->slot[packet->i];

	/* clear the forward bit */
	slot->flags &= ~NS_FORWARD;

	/* drop kernel reference */
	source->cur = NETMAP_RING_NEXT(source, packet->i);
	--source->avail;

	/* drop userland reference */
	NETPKT_PUT(packet);

	return (0);
}

unsigned int
peak_netmap_divert(struct peak_netmap *u_packet, const char *ifname)
{
	/* assume the user is sane enough to not pass NULL */
	struct _peak_netmap *packet = NETPKT_FROM_USER(u_packet);
	struct netmap_ring *source = packet->ring;
	struct netmap_ring *drain;
	struct my_ring *dev;
	unsigned int i, tx;

	if (!source) {
		/* packet is empty */
		return (0);
	}

	i = peak_netmap_find(ifname);
	if (i >= NETMAP_COUNT()) {
		/* not found means error */
		return (1);
	}

	dev = self->dev[i];

	NR_FOREACH_TX(drain, tx, dev, 0) {
		struct netmap_slot *rs, *ts;
		uint32_t pkt;

		if (!drain->avail) {
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

		--source->avail;
		--drain->avail;

		source->cur = NETMAP_RING_NEXT(source, packet->i);
		drain->cur = NETMAP_RING_NEXT(drain, i);

		NETPKT_PUT(packet);

		return (0);
	}

	/* could not release packet, try again */
	return (1);
}

static void
peak_netmap_configure(struct my_ring *dev)
{
	struct netmap_ring *ring;
	unsigned int i;

	/*
	 * Set `transparent' mode in the RX rings.
	 * That'll allow us to indirectly forward
	 * packets from a receive ring to its final
	 * destination, unless we clear the NS_FORWARD
	 * flag for a slot in packet processing.
	 */

	NR_FOREACH_RX(ring, i, dev, 1) {
		ring->flags |= NR_FORWARD;
	}
}

unsigned int
peak_netmap_attach(const char *ifname)
{
	struct my_ring **pdev = NULL;
	struct my_ring *dev = NULL;
	struct pollfd *pfd = NULL;
	unsigned int i;

	NETMAP_LOCK();

	/* lazy init */
	if (!NETMAP_COUNT()) {
		bzero(self, sizeof(*self));

		prealloc_init(&self->pkt_pool, NETMAP_MAX,
		    sizeof(struct _peak_netmap));
		prealloc_init(&self->dev_pool, NETMAP_MAX,
		    sizeof(struct my_ring));
	}

	i = peak_netmap_find(ifname);
	if (i < NETMAP_COUNT()) {
		NETMAP_UNLOCK();
		alert("netmap interface %s already attached\n");
		return (1);
	}

	dev = NETMAP_GET();
	if (!dev) {
		NETMAP_UNLOCK();
		alert("netmap interface capacity reached\n");
		return (1);
	}

	bzero(dev, sizeof(*dev));

	pdev = &self->dev[NETMAP_COUNT() - 1];
	pfd = &self->fd[NETMAP_COUNT() - 1];

	dev->ifname = strdup(ifname);

	if (netmap_open(dev, 0, 1)) {
		free(dev->ifname);
		NETMAP_PUT(dev);
		NETMAP_UNLOCK();
		alert("could not open netmap device %s\n", ifname);
		return (1);
	}

	peak_netmap_configure(dev);

	*pdev = dev;

	pfd->fd = dev->fd;

	NETMAP_UNLOCK();

	return (0);
}

unsigned int
peak_netmap_detach(const char *ifname)
{
	struct my_ring *dev = NULL;
	struct my_ring **pdev = NULL;
	struct pollfd *pfd = NULL;
	unsigned int i;

	NETMAP_LOCK();

	i = peak_netmap_find(ifname);
	if (i >= NETMAP_COUNT()) {
		NETMAP_UNLOCK();
		alert("netmap interface %s not attached\n", ifname);
		return (1);
	}

	pdev = &self->dev[i];
	pfd = &self->fd[i];

	dev = *pdev;

	free(dev->ifname);
	netmap_close(dev);
	NETMAP_PUT(dev);

	if (i < NETMAP_COUNT()) {
		/* reshuffle list to make it linear */
		*pdev = self->dev[NETMAP_COUNT()];
		self->dev[NETMAP_COUNT()] = NULL;

		/* rebuild pollfd list as well */
		memcpy(pfd, &self->fd[NETMAP_COUNT()], sizeof(*pfd));
		bzero(&self->fd[NETMAP_COUNT()], sizeof(*pfd));
	}

	/* lazy exit */
	if (!NETMAP_COUNT()) {
		prealloc_exit(&self->pkt_pool);
		prealloc_exit(&self->dev_pool);
	}

	NETMAP_UNLOCK();

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
