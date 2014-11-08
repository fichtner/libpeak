# respect build order here!

SUBDIR=	include \
	contrib \
	lib \
	test \
	bin \
	config

sweep:
	@git ls-files | egrep -v "^(contrib|regress|sample)" | \
	    xargs -n1 scripts/cleanfile

.include <bsd.subdir.mk>
