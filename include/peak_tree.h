#ifndef PEAK_TREE_H
#define PEAK_TREE_H

typedef s32 (*peak_tree_compare_func) (void *u1, void *u2);
typedef u32 (*peak_tree_compare_funk) (const void *u1, const void *u2);

static peak_tree_compare_func peak_tree_compare = NULL;
static peak_tree_compare_funk __peak_tree_lt = NULL;
static peak_tree_compare_funk __peak_tree_eq = NULL;

struct peak_tree {
	struct peak_tree *t[2];
	u32 l, reserved;
};

static struct peak_tree peak_tree_sentinel = { { NULL, NULL }, 1, 0 };

#define NIL (&peak_tree_sentinel)

static inline void peak_tree_init(peak_tree_compare_funk lt, peak_tree_compare_funk eq)
{
	__peak_tree_lt = lt;
	__peak_tree_eq = eq;
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

static struct peak_tree *_peak_tree_insert(struct peak_tree *t, struct peak_tree *n)
{
	if (t != NIL) {
		const u32 dir = __peak_tree_lt(t, n);

		t->t[dir] = _peak_tree_insert(t->t[dir], n);

		return peak_tree_split(peak_tree_skew(t));
	}

	/* new leaf is easy */
	n->t[0] = n->t[1] = NIL;
	n->l = 2;

	return n;
}

static inline void *peak_tree_insert(void *t, void *n)
{
	/* because casting sucks */
	return _peak_tree_insert(t, n);
}

static struct peak_tree *_peak_tree_lookup(struct peak_tree *t, struct peak_tree *o)
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

#define PREDECESSOR	0
#define SUCCESSOR	1

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
	if (t == NIL) {
		return t;
	}
 
	if (__peak_tree_eq(t, o)) {
		if (t->t[0] == NIL) {
			if (t->t[1] == NIL) {
				return NIL;
			}

			t = peak_tree_leaf(t, SUCCESSOR);
		} else {
			t = peak_tree_leaf(t, PREDECESSOR);
		}
	} else {
		const u32 dir = __peak_tree_lt(t, o);
 
		t->t[dir] = _peak_tree_remove(t->t[dir], o);
	}

	{
		const u32 should_be = t->l - 1;

		if (t->t[0]->l < should_be || t->t[1]->l < should_be) {
			t->l = should_be;

			if (t->t[1]->l > should_be) {
				t->t[1]->l = should_be;
			}

			t = peak_tree_split(peak_tree_skew(t));
		}
	}

	return t;
}

static inline void *peak_tree_remove(void *t, void *r)
{
	return _peak_tree_remove(t, r);
}

static u32 _peak_tree_height(struct peak_tree *t)
{
	u32 l = 0, r = 0;

	if (t != NIL) {
		l = _peak_tree_height(t->t[0]) + 1;
		r = _peak_tree_height(t->t[1]) + 1;
	}

	return r > l ? r : l;
}

static inline u32 peak_tree_height(void *t)
{
	return _peak_tree_height(t);
}

static u32 _peak_tree_count(struct peak_tree *t)
{
	if (t == NIL) {
		return 0;
	}

	return 1 + _peak_tree_count(t->t[0]) + _peak_tree_count(t->t[1]);
}

static inline u32 peak_tree_count(void *t)
{
	return _peak_tree_count(t);
}

#endif /* PEAK_TREE_H */
