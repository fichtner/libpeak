#include <peak.h>

#define TRACK_SIZE(x)	(sizeof((x)->user) + sizeof((x)->port) +	\
    sizeof((x)->type))

#define TRACK_CMP(x, y)	memcmp(x, y, TRACK_SIZE(x))

RB_HEAD(peak_track_tree, peak_track);

struct peak_tracks {
	struct peak_track_tree flows;
	TAILQ_HEAD(, peak_track) tos;
	prealloc_t mem;
};

RB_GENERATE_STATIC(peak_track_tree, peak_track, rb_track, TRACK_CMP);

struct peak_track *
peak_track_acquire(struct peak_tracks *self, const struct peak_track *ref)
{
	static uint64_t next_flow_id = 0;
	struct peak_track *flow;

	if (unlikely(!ref)) {
		return (NULL);
	}

	flow = RB_FIND(peak_track_tree, &self->flows, ref);
	if (likely(flow)) {
		TAILQ_REMOVE(&self->tos, flow, tq_to);
		TAILQ_INSERT_TAIL(&self->tos, flow, tq_to);

		return (flow);
	}

	if (prealloc_empty(&self->mem)) {
		flow = TAILQ_FIRST(&self->tos);
		TAILQ_REMOVE(&self->tos, flow, tq_to);
		RB_REMOVE(peak_track_tree, &self->flows, flow);
		prealloc_put(&self->mem, flow);
	}

	flow = prealloc_get(&self->mem);
	if (unlikely(!flow)) {
		panic("flow pool empty\n");
	}

	bzero(flow, sizeof(*flow));
	bcopy(ref, flow, TRACK_SIZE(ref));

	if (unlikely(RB_INSERT(peak_track_tree, &self->flows, flow))) {
		panic("can't insert flow\n");
	}

	flow->id = __sync_fetch_and_add(&next_flow_id, 1);
	TAILQ_INSERT_TAIL(&self->tos, flow, tq_to);

	return (flow);
}

struct peak_tracks *
peak_track_init(const size_t max_flows)
{
	struct peak_tracks *self = zalloc(sizeof(*self));

	if (!self) {
		return (NULL);
	}

	if (!prealloc_init(&self->mem, max_flows,
	    sizeof(struct peak_track))) {
		free(self);
		return (NULL);
	}

	TAILQ_INIT(&self->tos);
	RB_INIT(&self->flows);

	return (self);
}

void
peak_track_exit(struct peak_tracks *self)
{
	struct peak_track *flow;

	if (!self) {
		return;
	}

	while ((flow = RB_ROOT(&self->flows))) {
		RB_REMOVE(peak_track_tree, &self->flows, flow);
		prealloc_put(&self->mem, flow);
	}

	prealloc_exit(&self->mem);
	free(self);
}