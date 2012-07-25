#ifndef PEAK_TREE_H
#define PEAK_TREE_H

typedef s32 (*peak_tree_compare_func) (void *u1, void *u2);

static peak_tree_compare_func peak_tree_compare = NULL;

struct peak_tree {
	struct peak_tree *t[2];
	u32 l, reserved;
};

static struct peak_tree peak_tree_sentinel = { { NULL, NULL }, 1, 0 };

#define NIL (&peak_tree_sentinel)

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
	if (t == NIL) {
		n->t[0] = n->t[1] = NIL;
		n->l = 2;
		t = n;
	} else {
		s32 ret = peak_tree_compare(n, t);
		if (ret < 0) {
			t->t[0] = _peak_tree_insert(t->t[0], n);
		} else if (ret > 0) {
			t->t[1] = _peak_tree_insert(t->t[1], n);
		}
	}

	return peak_tree_split(peak_tree_skew(t));
}

static inline void *peak_tree_insert(void *t, void *n)
{
	/* because casting sucks */
	return _peak_tree_insert(t, n);
}

static struct peak_tree *_peak_tree_lookup(struct peak_tree *t, struct peak_tree *o)
{
	while (t != NIL) {
		s32 ret = peak_tree_compare(o, t);
		if (!ret) {
			return t;
		}

		t = ret < 0 ? t->t[0] : t->t[1];
	}

	return NULL;
}

static inline void *peak_tree_lookup(void *t, void *o)
{
	/* (most of the time) */
	return _peak_tree_lookup(t, o);
}

static inline struct peak_tree *peak_tree_successor(struct peak_tree *t)
{
	struct peak_tree **s = &t->t[1];
	struct peak_tree *ret;

	while ((*s)->t[0] != NIL) {
		s = &(*s)->t[0];
	}

	ret = *s; 
	*s = NIL;

	if (ret != t->t[1]) {
		ret->t[1] = t->t[1];
	}

	ret->t[0] = t->t[0];
	ret->l = t->l;

	return ret;
}

static inline struct peak_tree *peak_tree_predecessor(struct peak_tree *t)
{
	struct peak_tree **p = &t->t[0];
	struct peak_tree *ret;

	while ((*p)->t[1] != NIL) {
		p = &(*p)->t[1];
	}

	ret = *p;
	*p = NIL;

	if (ret != t->t[0]) {
		ret->t[0] = t->t[0];
	}

	ret->t[1] = t->t[1];
	ret->l = t->l;

	return ret;
}

static struct peak_tree *_peak_tree_remove(struct peak_tree *t, struct peak_tree *o)
{
	s32 ret;

	if (t == NIL) {
		return t;
	}

	ret = peak_tree_compare(o, t);
	if (ret < 0) {
		t->t[0] = _peak_tree_remove(t->t[0], o);
	} else if (ret > 0) {
		t->t[1] = _peak_tree_remove(t->t[1], o);
	} else {
		if (t->t[0] == NIL) {
			if (t->t[1] == NIL) {
				return NIL;
			}

			t = peak_tree_successor(t);
		} else {
			t = peak_tree_predecessor(t);
		}
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
