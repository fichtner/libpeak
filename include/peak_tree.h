#ifndef PEAK_TREE_H
#define PEAK_TREE_H

typedef s32 (*peak_tree_compare_func) (void *u1, void *u2);

static peak_tree_compare_func peak_tree_compare = NULL;

struct peak_tree {
	struct peak_tree *right;
	struct peak_tree *left;
	u32 level, reserved;
};

static struct peak_tree peak_tree_sentinel = { NULL, NULL, 1, 0 };

#define NIL (&peak_tree_sentinel)

static inline struct peak_tree *peak_tree_skew(struct peak_tree *t)
{
	if (t != NIL && t->left->level == t->level) {
		struct peak_tree *l = t->left;

		t->left = l->right;
		l->right = t;

		return l;
	}

	return t;
}

static inline struct peak_tree *peak_tree_split(struct peak_tree *t)
{
	if (t != NIL && t->right != NIL && t->right->right->level == t->level) {
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
	if (t == NIL) {
		n->left = n->right = NIL;
		n->level = 2;
		t = n;
	} else {
		s32 ret = peak_tree_compare(n, t);
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
	while (t != NIL) {
		s32 ret = peak_tree_compare(o, t);
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

static inline struct peak_tree *peak_tree_successor(struct peak_tree *t)
{
	struct peak_tree **s = &t->right;
	struct peak_tree *ret;

	while ((*s)->left != NIL) {
		s = &(*s)->left;
	}

	ret = *s; 
	*s = NIL;

	if (ret != t->right) {
		ret->right = t->right;
	}

	ret->level = t->level;
	ret->left = t->left;

	return ret;
}

static inline struct peak_tree *peak_tree_predecessor(struct peak_tree *t)
{
	struct peak_tree **p = &t->left;
	struct peak_tree *ret;

	while ((*p)->right != NIL) {
		p = &(*p)->right;
	}

	ret = *p;
	*p = NIL;

	if (ret != t->left) {
		ret->left = t->left;
	}

	ret->level = t->level;
	ret->right = t->right;

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
		t->left = _peak_tree_remove(t->left, o);
	} else if (ret > 0) {
		t->right = _peak_tree_remove(t->right, o);
	} else {
		if (t->left == NIL) {
			if (t->right == NIL) {
				return NIL;
			}

			t = peak_tree_successor(t);
		} else {
			t = peak_tree_predecessor(t);
		}
	}

	if (t->left->level < t->level - 1 || t->right->level < t->level - 1) {
		if (t->right->level > --t->level) {
			t->right->level = t->level;
		}

		t = peak_tree_split(peak_tree_skew(t));
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
		l = _peak_tree_height(t->left) + 1;
		r = _peak_tree_height(t->right) + 1;
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

	return 1 + _peak_tree_count(t->left) + _peak_tree_count(t->right);
}

static inline u32 peak_tree_count(void *t)
{
	return _peak_tree_count(t);
}

#endif /* PEAK_TREE_H */
