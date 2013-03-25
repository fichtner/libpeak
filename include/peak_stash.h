#ifndef PEAK_STASH_H
#define PEAK_STASH_H

#define STASH_DECLARE(name, type, count)				\
struct {								\
	unsigned int index;						\
	unsigned int reserved;						\
	type values[count];						\
} name = {								\
	.index = 0,	/* must be zero */				\
}

#define STASH_FULL(name)	(name.index >= lengthof(name.values))
#define STASH_EMPTY(name)	(!name.index)
#define STASH_COUNT(name)	(name.index)

#define STASH_CLEAR(name) do {						\
	name.index = 0;							\
} while (0)

#define STASH_PUSH(x, name) do {					\
	if (unlikely(STASH_FULL(name))) {				\
		panic("stash storage full\n");				\
	}								\
	name.values[name.index++] = *(x);				\
} while (0)

#define STASH_POP(name) ({	 					\
	if (unlikely(STASH_EMPTY(name))) {				\
		panic("stash storage empty\n");				\
	}								\
	&name.values[--name.index];					\
})

#define STASH_FOREACH(x, name)						\
	if (likely(!STASH_EMPTY(name)))					\
		for ((x) = &name.values[0];				\
		     (x) < &name.values[STASH_COUNT(name)];		\
		     (x)++)

#define STASH_FOREACH_REVERSE(x, name)					\
	if (likely(!STASH_EMPTY(name)))					\
		for ((x) = &name.values[STASH_COUNT(name) - 1];		\
		     (x) >= &name.values[0];				\
		     (x)--)

#endif /* !PEAK_STASH_H */
