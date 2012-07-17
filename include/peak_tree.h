#ifndef PEAK_TREE_H
#define PEAK_TREE_H

typedef s32 (*peak_tree_compare) (void *u1, void *u2);

static peak_tree_compare __cmp__ = NULL;

struct peak_tree {
	struct peak_tree *right;
	struct peak_tree *left;
	u32 level, reserved;
};

static struct peak_tree sentinel = { NULL, NULL, 1, 0 };

static inline struct peak_tree *peak_tree_skew(struct peak_tree *t)
{
	if (t->level > 1 && t->left->level == t->level) {
		struct peak_tree *l = t->left;

		t->left = l->right;
		t->right = t;

		return l;
	}

	return t;
}

static inline struct peak_tree *peak_tree_split(struct peak_tree *t)
{
	if (t->right->level > 1 && t->right->right->level == t->level) {
		struct peak_tree *r = t->right;

		t->right = r->left;
		r->left = t;
		++r->level;

		return r;
	}

	return t;
}

static struct peak_tree *_peak_tree_insert(struct peak_tree *t, struct peak_tree *n)
{
	if (t == &sentinel) {
		n->left = n->right = &sentinel;
		n->level = 2;
		t = n;
	} else {
		s32 ret = __cmp__(n, t);
		if (ret < 0) {
			t->left = _peak_tree_insert(t->left, n);
		} else if (ret > 0) {
			t->right = _peak_tree_insert(t->right, n);
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
	while (t != &sentinel) {
		s32 ret = __cmp__(o, t);
		if (!ret) {
			return t;
		}

		t = ret < 0 ? t->left : t->right;
	}

	return NULL;
}

static inline void *peak_tree_lookup(void *t, void *o)
{
	/* (most of the time) */
	return _peak_tree_lookup(t, o);
}

#endif /* PEAK_TREE_H */
