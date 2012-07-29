#ifndef PEAK_TREE_H
#define PEAK_TREE_H

/* General thoughts on this AA tree implementation:
 * 
 *  - The best performance of these trees seems to be in the 5k
 *    to 10k nodes range. Anything else is just wrecking inserts
 *    and removes.
 *  - Lookups are generally faster, because they don't have to
 *    do any rebalance stuff. I can't say how much, but then you
 *    really shouldn't worry about it in the first place.
 *  - In the past, I have used these trees mainly for storage
 *    and retrieval. Once the data was outdated, the whole thing
 *    was just being dissolved, so there wasn't any need to do
 *    sophisticated removes.
 *  - This implementation uses embedded structures and compare
 *    function pointers to provide better real world applications
 *    with complex value sets and private data.
 *  - Locks are not provided and need to be implemented by the user.
 */

typedef u32 (*peak_tree_compare_funk) (const void *u1, const void *u2);
typedef void (*peak_tree_callback_funk) (void *ptr, void *ctx);

static peak_tree_compare_funk __peak_tree_lt = NULL;
static peak_tree_compare_funk __peak_tree_eq = NULL;

struct peak_tree {
	struct peak_tree *t[2];
	u32 l, reserved;
};

static struct peak_tree __peak_tree_sentinel = { { NULL, NULL }, 1, 0 };

#define NIL (&__peak_tree_sentinel)

static inline void peak_tree_init(peak_tree_compare_funk lt, peak_tree_compare_funk eq)
{
	__peak_tree_lt = lt;
	__peak_tree_eq = eq;

	__peak_tree_sentinel.t[0] = NIL;
	__peak_tree_sentinel.t[1] = NIL;
}

static inline struct peak_tree *peak_tree_skew(struct peak_tree *t)
{
	if (t != NIL && t->t[0]->l == t->l) {
		struct peak_tree *l = t->t[0];

		t->t[0] = l->t[1];
		l->t[1] = t;

		return l;
	}

	return t;
}

static inline struct peak_tree *peak_tree_split(struct peak_tree *t)
{
	if (t != NIL && t->t[1] != NIL && t->t[1]->t[1]->l == t->l) {
		struct peak_tree *r = t->t[1];

		t->t[1] = r->t[0];
		r->t[0] = t;
		++r->l;

		return r;
	}

	return t;
}

#define PEAK_TREE_STACK_SIZE 32

static struct peak_tree *_peak_tree_insert(struct peak_tree *t, struct peak_tree *n)
{
	struct peak_tree *h[PEAK_TREE_STACK_SIZE];
	u32 i = 0;

	if (t == NIL) {
		n->t[0] = n->t[1] = NIL;
		n->l = 2;
		return n;
	}

	for (;;) {
		const u32 dir = __peak_tree_lt(t, n);

		if (PEAK_TREE_STACK_SIZE == i) {
			peak_panic("stack overflow\n");
			/* although it would be funny to fall back
			 * to recursive mode in this case... :) */
		}

		h[i++] = t;

		if (t->t[dir] == NIL) {
			n->t[0] = n->t[1] = NIL;
			t->t[dir] = n;
			n->l = 2;
			--i;
			break;
		}

		t = t->t[dir];
	}

	for (;;) {
		t = peak_tree_split(peak_tree_skew(t));

		if (!i--) {
			break;
		}

		/* quite dodgy: propagate new subtree upwards */
		h[i]->t[h[i]->t[1] == h[i + 1]] = t;
		t = h[i];
	}

	return t;
}

static inline void *peak_tree_insert(void *t, void *n)
{
	/* because casting sucks */
	return _peak_tree_insert(t, n);
}

static inline struct peak_tree *_peak_tree_lookup(struct peak_tree *t, struct peak_tree *o)
{
	struct peak_tree *c = NIL;

	while (t != NIL) {
		if (__peak_tree_lt(o, t)) {
			t = t->t[0];
		} else {
			c = t;
			t = t->t[1];
		}
	}

	if (c != NIL && __peak_tree_eq(o, c)) {
		return c;
	}

	return NIL;
}

static inline void *peak_tree_lookup(void *t, void *o)
{
	/* (most of the time) */
	return _peak_tree_lookup(t, o);
}

static inline struct peak_tree *peak_tree_leaf(struct peak_tree *t, const u32 dir)
{
	struct peak_tree **l = &t->t[dir];
	struct peak_tree *ret;

	while ((*l)->t[!dir] != NIL) {
		l = &(*l)->t[!dir];
	}

	ret = *l; 
	*l = NIL;

	if (ret != t->t[dir]) {
		ret->t[dir] = t->t[dir];
	}

	ret->t[!dir] = t->t[!dir];
	ret->l = t->l;

	return ret;
}

static struct peak_tree *_peak_tree_remove(struct peak_tree *t, struct peak_tree *o)
{
	struct peak_tree *h[PEAK_TREE_STACK_SIZE];
	u32 i = 0;

	if (t == NIL) {
		return t;
	}

	for (;;) {
		if (PEAK_TREE_STACK_SIZE == i) {
			peak_panic("stack overflow\n");
		}

		h[i++] = t;

		/* that's a shortcut: we expect the caller to know the
		 * object it wants to remove from the tree structure! */ 
		if (t == o) {
			--i;
			break;
		}

		t = t->t[__peak_tree_lt(t, o)];
		if (t == NIL) {
			return h[0];
		}
	}

	if (t->t[0] == NIL && t->t[1] == NIL) {
		t = NIL;
	} else {
		/* instead of switching the node data, we'll do
		 * a full transform into a leaf case, which is
		 * a bit more expensive, but we don't know the
		 * size of the actual data in this implementation! */
		t = peak_tree_leaf(t, t->t[1] != NIL);
	}

	for (;;) {
		const u32 should_be = t->l - 1;

		if (t->t[0]->l < should_be || t->t[1]->l < should_be) {
			t->l = should_be;

			if (t->t[1]->l > should_be) {
				t->t[1]->l = should_be;
			}

			t = peak_tree_skew(t);
			t->t[1] = peak_tree_skew(t->t[1]);
			t->t[1]->t[1] = peak_tree_skew(t->t[1]->t[1]);
			t = peak_tree_split(t);
			t->t[1] = peak_tree_split(t->t[1]);
		}

		if (!i--) {
			break;
		}

		/* still not pretty: propagate new subtree upwards */
		h[i]->t[h[i]->t[1] == h[i + 1]] = t;
		t = h[i];
	}

	return t;
}

static void _peak_tree_collapse(struct peak_tree *t, peak_tree_callback_funk cb, void *ctx)
{
	if (t == NIL) {
		return;
	}

	_peak_tree_collapse(t->t[0], cb, ctx);
	_peak_tree_collapse(t->t[1], cb, ctx);

	t->t[0] = t->t[1] = NIL;

	cb(t, ctx);
}

static void __peak_tree_callback_empty(void *ptr, void *ctx)
{
	(void) ptr;
	(void) ctx;
}

static inline void *peak_tree_collapse(void *t, peak_tree_callback_funk cb, void *ctx)
{
	_peak_tree_collapse(t, cb ? : __peak_tree_callback_empty, ctx);

	return NIL;
}

static inline void *peak_tree_remove(void *t, void *r)
{
	return _peak_tree_remove(t, r);
}

static u32 _peak_tree_height(const struct peak_tree *t)
{
	u32 l = 0, r = 0;

	if (t != NIL) {
		l = _peak_tree_height(t->t[0]) + 1;
		r = _peak_tree_height(t->t[1]) + 1;
	}

	return r > l ? r : l;
}

static inline u32 peak_tree_height(const void *t)
{
	return _peak_tree_height(t);
}

static u32 _peak_tree_count(const struct peak_tree *t)
{
	const struct peak_tree *h[PEAK_TREE_STACK_SIZE];
	u32 i = 0, count = 0;

	for (;;) {
		if (t != NIL) {
			if (PEAK_TREE_STACK_SIZE == i) {
				peak_panic("stack overflow\n");
			}
			h[i++] = t;
			t = t->t[0];
		} else {
			if (!i) {
				break;
			}

			t = h[--i];
			t = t->t[1];
			++count;
		}
	}

	return count;
}

static inline u32 peak_tree_count(const void *t)
{
	return _peak_tree_count(t);
}

#endif /* PEAK_TREE_H */
