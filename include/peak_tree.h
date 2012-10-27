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

#define TO_NULL(name, x)	((x) != NIL(name) ? (x) : NULL)
#define TO_NIL(name, x)		((x) ? : NIL(name))
#define NIL(name)		&name##_AA_SENTINEL

#define AA_HEAD(type)							\
struct type *

#define AA_ENTRY(type)							\
struct {								\
	struct type *aae_child[2];	/* left/right element */	\
	unsigned int aae_level;		/* node level */		\
}

#define AA_CHILD(elm, field, dir)	(elm)->field.aae_child[dir]
#define AA_LEFT(elm, field)		AA_CHILD(elm, field, 0)
#define AA_RIGHT(elm, field)		AA_CHILD(elm, field, 1)
#define AA_LEVEL(elm, field)		(elm)->field.aae_level
#define AA_ROOT(head)			*(head)
#define AA_EMPTY(head)			(AA_ROOT(head) == NULL)
#define AA_STACK_SIZE			32

#define AA_INIT(name, head, field) do {					\
	AA_LEFT(NIL(name), field) = NIL(name);				\
	AA_RIGHT(NIL(name), field) = NIL(name);				\
	AA_LEVEL(NIL(name), field) = 1;					\
	AA_ROOT(head) = NULL;						\
} while (0)

#define AA_SKEW(elm, tmp, field) do {					\
	if (AA_LEVEL(AA_LEFT(elm, field), field) ==			\
	    AA_LEVEL(elm, field)) {					\
		(tmp) = AA_LEFT(elm, field);				\
		AA_LEFT(elm, field) = AA_RIGHT(tmp, field);		\
		AA_RIGHT(tmp, field) = (elm);				\
	} else {							\
		(tmp) = (elm);						\
	}								\
} while (0)

#define AA_SPLIT(elm, tmp, field) do {					\
	if (AA_LEVEL(AA_RIGHT(AA_RIGHT(elm, field), field), field) ==	\
	    AA_LEVEL(elm, field)) {					\
		(tmp) = AA_RIGHT(elm, field);				\
		AA_RIGHT(elm, field) = AA_LEFT(tmp, field);		\
		AA_LEFT(tmp, field) = (elm);				\
		++AA_LEVEL(tmp, field);					\
	} else {							\
		(tmp) = (elm);						\
	}								\
} while (0)

#define AA_GENERATE(name, type, field, lt, eq)				\
	AA_GENERATE_INTERNAL(name, type, field, lt, eq,)
#define AA_GENERATE_STATIC(name, type, field, lt, eq)			\
	AA_GENERATE_INTERNAL(name, type, field, lt, eq,			\
	    __attribute__((__unused__)) static)
#define AA_GENERATE_INTERNAL(name, type, field, lt, eq, attr)		\
typedef void (*AA_COLLAPSE_FUNK) (struct type *elm, void *ctx);		\
attr struct type name##_AA_SENTINEL;					\
attr inline struct type *						\
name##_AA_INSERT_INTERNAL(struct type *cur, struct type *elm)		\
{									\
	struct type *h[AA_STACK_SIZE];					\
	struct type *tmp;						\
	unsigned int i = 0;						\
									\
	if ((cur) == NIL(name)) {					\
		AA_LEFT(elm, field) = AA_RIGHT(elm, field) = NIL(name);	\
		AA_LEVEL(elm, field) = 2;				\
		return (elm);						\
	}								\
									\
	for (;;) {							\
		const unsigned int dir = lt(cur, elm);			\
									\
		if (AA_STACK_SIZE == i) {				\
			peak_panic("stack overflow\n");			\
			/* although it would be funny to fall back	\
			 * to recursive mode in this case... :) */	\
		}							\
									\
		h[i++] = (cur);						\
									\
		if (AA_CHILD(cur, field, dir) == NIL(name)) {		\
			AA_LEFT(elm, field) =				\
			    AA_RIGHT(elm, field) = NIL(name);		\
			AA_CHILD(cur, field, dir) = (elm);		\
			AA_LEVEL(elm, field) = 2;			\
			--i;						\
			break;						\
		}							\
									\
		(cur) = AA_CHILD(cur, field, dir);			\
	}								\
									\
	for (;;) {							\
		AA_SKEW(cur, tmp, field);				\
		(cur) = (tmp);						\
		AA_SPLIT(cur, tmp, field);				\
		(cur) = (tmp);						\
									\
		if (!i--) {						\
			break;						\
		}							\
									\
		/* quite dodgy: propagate new subtree upwards */	\
		AA_CHILD(h[i], field, AA_RIGHT(h[i], field) ==		\
		    h[i + 1]) =	(cur);					\
		(cur) = h[i];						\
	}								\
									\
	return (tmp);							\
}									\
									\
attr struct type *							\
name##_AA_INSERT(struct type *t, struct type *n)			\
{									\
	void *ret = name##_AA_INSERT_INTERNAL(TO_NIL(name, t),		\
	    TO_NIL(name, n));						\
									\
	/* because casting sucks */					\
	return (TO_NULL(name, ret));					\
}									\
									\
attr inline struct type *						\
name##_AA_FIND_INTERNAL(struct type *t, struct type *o)			\
{									\
	struct type *c = NIL(name);					\
									\
	while (t != NIL(name)) {					\
		if (lt(o, t)) {						\
			t = AA_LEFT(t, field);				\
		} else {						\
			c = t;						\
			t = AA_RIGHT(t, field);				\
		}							\
	}								\
									\
	if (c != NIL(name) && eq(o, c)) {				\
		return (c);						\
	}								\
									\
	return (NIL(name));						\
}									\
									\
attr inline struct type *						\
name##_AA_FIND(struct type *t, struct type *o)				\
{									\
	void *ret = name##_AA_FIND_INTERNAL(TO_NIL(name, t),		\
	    TO_NIL(name, o));						\
									\
	/* (most of the time) */					\
	return (TO_NULL(name, ret));					\
}									\
									\
attr inline struct type *						\
name##_AA_LEAF(struct type *cur, const unsigned int dir)		\
{									\
	struct type **l = &AA_CHILD(cur, field, dir);			\
	struct type *ret;						\
									\
	while (AA_CHILD(*l, field, !dir) != NIL(name)) {		\
		l = &AA_CHILD(*l, field, !dir);				\
	}								\
									\
	(ret) = *l;							\
	*l = NIL(name);							\
									\
	if ((ret) != AA_CHILD(cur, field, dir)) {			\
		AA_CHILD(ret, field, dir) = AA_CHILD(cur, field, dir);	\
	}								\
									\
	AA_CHILD(ret, field, !dir) = AA_CHILD(cur, field, !dir);	\
	AA_LEVEL(ret, field) = AA_LEVEL(cur, field);			\
									\
	return (ret);							\
}									\
									\
attr struct type *							\
name##_AA_REMOVE_INTERNAL(struct type *cur, struct type *elm)		\
{									\
	struct type *h[AA_STACK_SIZE];					\
	struct type *tmp;						\
	unsigned int i = 0;						\
									\
	if ((cur) == NIL(name)) {					\
		return NIL(name);					\
	}								\
									\
	for (;;) {							\
		if (AA_STACK_SIZE == i) {				\
			peak_panic("stack overflow\n");			\
		}							\
									\
		h[i++] = (cur);						\
									\
		/* that's a shortcut: we expect the caller		\
		 * to know the object it wants to remove		\
		 * from the tree structure! */				\
		if ((cur) == (elm)) {					\
			--i;						\
			break;						\
		}							\
									\
		(cur) = AA_CHILD(cur, field, lt(cur, elm));		\
		if ((cur) == NIL(name)) {				\
			return (h[0]);					\
		}							\
	}								\
									\
	if (AA_LEFT(cur, field) == NIL(name) &&				\
	    AA_RIGHT(cur, field) == NIL(name)) {			\
		(cur) = NIL(name);					\
	} else {							\
		/* instead of switching the node data, we'll do		\
		 * a full transform into a leaf case, which is		\
		 * a bit more expensive, but we don't know the		\
		 * size of the actual data in this implementation! */	\
		(cur) = name##_AA_LEAF(cur,				\
		    AA_RIGHT(cur, field) != NIL(name));			\
	}								\
									\
	for (;;) {							\
		const unsigned int should_be =				\
		    AA_LEVEL(cur, field) - 1;				\
									\
		if (AA_LEVEL(AA_LEFT(cur, field), field) < should_be ||	\
		    AA_LEVEL(AA_RIGHT(cur, field), field) <		\
		    should_be) {					\
			AA_LEVEL(cur, field) = should_be;		\
									\
			if (AA_LEVEL(AA_RIGHT(cur, field), field) >	\
			    should_be) {				\
				AA_LEVEL(AA_RIGHT(cur, field), field) =	\
				    should_be;				\
			}						\
									\
			AA_SKEW(cur, tmp, field);			\
			(cur) = (tmp);					\
			AA_SKEW(AA_RIGHT(cur, field), tmp, field);	\
			AA_RIGHT(cur, field) = (tmp);			\
			AA_SKEW(AA_RIGHT(AA_RIGHT(cur, field), field),	\
			    tmp, field);				\
			AA_RIGHT(AA_RIGHT(cur, field), field) = (tmp);	\
			AA_SPLIT(cur, tmp, field);			\
			(cur) = (tmp);					\
			AA_SPLIT(AA_RIGHT(cur, field), tmp, field);	\
			AA_RIGHT(cur, field) = (tmp);			\
		}							\
									\
		if (!i--) {						\
			break;						\
		}							\
									\
		/* still not pretty: propagate new subtree upwards */	\
		AA_CHILD(h[i], field, AA_RIGHT(h[i], field) ==		\
		    h[i + 1]) = (cur);					\
		(cur) = h[i];						\
	}								\
									\
	return (cur);							\
}									\
									\
attr inline struct type *						\
name##_AA_REMOVE(struct type *t, struct type *r)			\
{									\
	void *ret = name##_AA_REMOVE_INTERNAL(TO_NIL(name, t),		\
	    TO_NIL(name, r));						\
									\
	return (TO_NULL(name, ret));					\
}									\
									\
attr void								\
name##_AA_COLLAPSE_INTERNAL(struct type *elm,				\
	AA_COLLAPSE_FUNK cb, void *ctx)					\
{									\
	struct type *h[AA_STACK_SIZE];					\
	unsigned int i = 0;						\
									\
	for (;;) {							\
		if (elm != NIL(name)) {					\
			if (AA_STACK_SIZE == i) {			\
				peak_panic("stack overflow\n");		\
			}						\
			h[i++] = elm;					\
			elm = AA_LEFT(elm, field);			\
		} else {						\
			if (!i) {					\
				break;					\
			}						\
									\
			elm = h[--i];					\
			elm = AA_RIGHT(elm, field);			\
									\
			{						\
				struct type *tmp = h[i];		\
									\
				AA_LEFT(tmp, field) =			\
				    AA_RIGHT(tmp, field) = NIL(name);	\
				cb(tmp, ctx);				\
			}						\
		}							\
	}								\
}									\
									\
attr void								\
name##_AA_COLLAPSE_EMPTY(struct type *elm, void *ctx)			\
{									\
	(void) elm;							\
	(void) ctx;							\
}									\
									\
attr inline struct type *						\
name##_AA_COLLAPSE(struct type *t, AA_COLLAPSE_FUNK cb, void *ctx)	\
{									\
	name##_AA_COLLAPSE_INTERNAL(TO_NIL(name, t),			\
	    cb ? : name##_AA_COLLAPSE_EMPTY, ctx);			\
									\
	return (NULL);							\
}									\
									\
attr unsigned int							\
name##_AA_HEIGHT_INTERNAL(const struct type *elm)			\
{									\
	unsigned int l = 0, r = 0;					\
									\
	if (elm != NIL(name)) {						\
		l = name##_AA_HEIGHT_INTERNAL(AA_LEFT(elm,		\
		    field)) + 1;					\
		r = name##_AA_HEIGHT_INTERNAL(AA_RIGHT(elm,		\
		    field)) + 1;					\
	}								\
									\
	return (r > l ? r : l);						\
}									\
									\
attr inline unsigned int						\
name##_AA_HEIGHT(const struct type *elm)				\
{									\
	return (name##_AA_HEIGHT_INTERNAL(TO_NIL(name, elm)));		\
}									\
									\
attr unsigned int							\
name##_AA_COUNT_INTERNAL(const struct type *elm)			\
{									\
	const struct type *h[AA_STACK_SIZE];				\
	unsigned int i = 0, count = 0;					\
									\
	for (;;) {							\
		if (elm != NIL(name)) {					\
			if (AA_STACK_SIZE == i) {			\
				peak_panic("stack overflow\n");		\
			}						\
			h[i++] = elm;					\
			elm = AA_LEFT(elm, field);			\
		} else {						\
			if (!i) {					\
				break;					\
			}						\
									\
			elm = h[--i];					\
			elm = AA_RIGHT(elm, field);			\
			++count;					\
		}							\
	}								\
									\
	return (count);							\
}									\
									\
attr inline unsigned int						\
name##_AA_COUNT(const struct type *elm)					\
{									\
	return (name##_AA_COUNT_INTERNAL(TO_NIL(name, elm)));		\
}

#define AA_FIND(name, x, y)		name##_AA_FIND(AA_ROOT(x), y)
#define AA_HEIGHT(name, x)		name##_AA_HEIGHT(AA_ROOT(x))
#define AA_COUNT(name, x)		name##_AA_COUNT(AA_ROOT(x))

#define AA_INSERT(name, x, y) do {					\
	AA_ROOT(x) = name##_AA_INSERT(AA_ROOT(x), y);			\
} while (0)

#define AA_REMOVE(name, x, y) do {					\
	AA_ROOT(x) = name##_AA_REMOVE(AA_ROOT(x), y);			\
} while (0)

#define AA_COLLAPSE(name, x, y, z) do {					\
	AA_ROOT(x) = name##_AA_COLLAPSE(AA_ROOT(x), y, z);		\
} while (0)

#endif /* !PEAK_TREE_H */
