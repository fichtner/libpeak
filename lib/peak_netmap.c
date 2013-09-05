/*
 * Copyright (c) 2013 Franco Fichtner <franco@packetwerk.com>
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

#define NETMAP_PUT(x)		prealloc_put(&self->me_pool, x)
#define NETMAP_COUNT()		prealloc_used(&self->me_pool)
#define NETMAP_GET()		prealloc_get(&self->me_pool)

static pthread_mutex_t netmap_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct peak_netmaps {
	prealloc_t pkt_pool;
	prealloc_t me_pool;
	unsigned int busy;
	unsigned int reserved;
	struct my_ring *me[NETMAP_MAX];
	struct pollfd fd[NETMAP_MAX];
} netmap_self;

static struct peak_netmaps *const self = &netmap_self;	/* global ref */

struct _peak_netmap {
	struct netmap_ring *ring;
	unsigned int reserved;
	unsigned int i;
	struct peak_netmap data;
};

static inline unsigned int
peak_netmap_find(const char *ifname)
{
	unsigned int i;

	if (!ifname) {
		return (NETMAP_COUNT());
	}

	for (i = 0; i < NETMAP_COUNT(); ++i) {
		if (!strcmp(ifname, self->me[i]->ifname)) {
			break;
		}
	}

	return (i);
}

static struct peak_netmap *
_peak_netmap_claim(void)
{
	struct _peak_netmap *packet;
	struct netmap_ring *ring;
	struct my_ring *me;
	unsigned int j, si;

	for (j = 0; j < NETMAP_COUNT(); ++j) {
		me = self->me[j];

		for (si = me->begin; si < me->end; ++si) {
			unsigned int i, idx;

			ring = NETMAP_RXRING(me->nifp, si);
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
				    me->nifp->ni_name, idx, i);
			}

			/* volatile internals */
			packet->ring = ring;
			packet->i = i;

			/* external stuff */
			packet->data.buf = NETMAP_BUF(ring, idx);
			packet->data.len = ring->slot[i].len;
			packet->data.ll = LINKTYPE_ETHERNET;
			packet->data.ts = ring->ts.tv_sec;
			packet->data.ifname = me->ifname;

			return (NETPKT_TO_USER(packet));
		}
	}

	return (NULL);
}

struct peak_netmap *
peak_netmap_claim(void)
{
	struct peak_netmap *packet;
	struct my_ring *me;
	struct pollfd *fd;
	unsigned int i;
	int ret;

	/*
	 * Look for packets prior to polling to avoid
	 * the system call overhead when not needed.
	 */

	packet = _peak_netmap_claim();
	if (packet) {
		return (packet);
	}

	/*
	 * Call poll() to relax the CPU while adapters are idle.
	 * We use a relatively low poll timeout of 200 ms to give
	 * the caller a sense of system responsiveness in order
	 * to do some work on its own (it may require
	 * synchronisation of threads or something likewise).
	 */

	for (i = 0; i < NETMAP_COUNT(); ++i) {
		fd = &self->fd[i];

		fd->events = POLLIN;
		fd->revents = 0;
	}

	ret = poll(self->fd, NETMAP_COUNT(), 200);
	if (ret <= 0) {
		/* error or timeout */
		return (NULL);
	}

	for (i = 0; i < NETMAP_COUNT(); ++i) {
		fd = &self->fd[i];
		me = self->me[i];

		if (fd->revents & POLLERR) {
			alert("error on %s, rxcur %d@%d", me->ifname,
			    me->rx->avail, me->rx->cur);
		}

		if (fd->revents & POLLIN) {
			/*
			 * XXX This is quite naive: We know the fd,
			 * but go through all interfaces anyway.
			 */
			return (_peak_netmap_claim());
		}
	}

	return (NULL);
}

static inline void
peak_netmap_drop(struct _peak_netmap *packet)
{
	struct netmap_ring *source = packet->ring;

	/* drop kernel reference */
	source->cur = NETMAP_RING_NEXT(source, packet->i);
	--source->avail;

	/* drop userland reference */
	NETPKT_PUT(packet);
}

unsigned int
peak_netmap_forward(struct peak_netmap *u_packet, const char *ifname)
{
	/* assume the user is sane enough to not pass NULL */
	struct _peak_netmap *packet = NETPKT_FROM_USER(u_packet);
	struct netmap_ring *source = packet->ring;
	struct netmap_ring *drain;
	struct my_ring *me;
	unsigned int i, di;

	if (!source) {
		/* packet is empty */
		return (0);
	}

	i = peak_netmap_find(ifname);
	if (i >= NETMAP_COUNT()) {
		/* not found means drop */
		peak_netmap_drop(packet);
		return (0);
	}

	me = self->me[i];

	for (di = me->begin; di < me->end; ++di) {
		struct netmap_slot *rs, *ts;
		uint32_t pkt;

		drain = NETMAP_TXRING(me->nifp, di);
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

unsigned int
peak_netmap_attach(const char *ifname)
{
	struct my_ring *slot = NULL;
	struct my_ring **me = NULL;
	struct pollfd *fd = NULL;
	unsigned int i;

	NETMAP_LOCK();

	/* lazy init */
	if (!NETMAP_COUNT()) {
		bzero(self, sizeof(*self));

		prealloc_init(&self->pkt_pool, NETMAP_MAX,
		    sizeof(struct _peak_netmap));
		prealloc_init(&self->me_pool, NETMAP_MAX,
		    sizeof(struct my_ring));
	}

	i = peak_netmap_find(ifname);
	if (i < NETMAP_COUNT()) {
		NETMAP_UNLOCK();
		alert("netmap interface %s already attached\n");
		return (1);
	}

	slot = NETMAP_GET();
	if (!slot) {
		NETMAP_UNLOCK();
		alert("netmap interface capacity reached\n");
		return (1);
	}

	bzero(slot, sizeof(*slot));

	me = &self->me[NETMAP_COUNT() - 1];
	fd = &self->fd[NETMAP_COUNT() - 1];

	slot->ifname = strdup(ifname);

	if (netmap_open(slot, 0, 1)) {
		free(slot->ifname);
		NETMAP_PUT(slot);
		NETMAP_UNLOCK();
		alert("could not open netmap device %s\n", ifname);
		return (1);
	}

	*me = slot;

	fd->fd = slot->fd;

	NETMAP_UNLOCK();

	return (0);
}

unsigned int
peak_netmap_detach(const char *ifname)
{
	struct my_ring *slot = NULL;
	struct my_ring **me = NULL;
	struct pollfd *fd = NULL;
	unsigned int i;

	NETMAP_LOCK();

	i = peak_netmap_find(ifname);
	if (i >= NETMAP_COUNT()) {
		NETMAP_UNLOCK();
		alert("netmap interface %s not attached\n", ifname);
		return (1);
	}

	me = &self->me[i];
	fd = &self->fd[i];

	slot = *me;

	free(slot->ifname);
	netmap_close(slot);

	NETMAP_PUT(slot);

	if (i < NETMAP_COUNT()) {
		/* reshuffle list to make it linear */
		*me = self->me[NETMAP_COUNT()];
		self->me[NETMAP_COUNT()] = NULL;

		/* rebuild pollfd list as well */
		bcopy(fd, &self->fd[NETMAP_COUNT()], sizeof(*fd));
		bzero(&self->fd[NETMAP_COUNT()], sizeof(*fd));
	}

	/* lazy exit */
	if (!NETMAP_COUNT()) {
		prealloc_exit(&self->pkt_pool);
		prealloc_exit(&self->me_pool);
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
