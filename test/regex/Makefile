PROG=	regex
MAN=

LDADD=	-lc -pthread
LDADD+=	$(.CURDIR)/../../lib/libpeak.a
LDADD+=	$(.CURDIR)/../../contrib/libcompat/libcompat.a
LDADD+= -lpcre

DPADD=	$(.CURDIR)/../../lib/libpeak.a
DPADD+=	$(.CURDIR)/../../contrib/libcompat/libcompat.a

.include <bsd.prog.mk>
