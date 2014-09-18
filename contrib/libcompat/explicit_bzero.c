/*	$OpenBSD: explicit_bzero.c,v 1.1 2011/01/10 23:23:56 tedu Exp $ */
/*
 * Public domain.
 * Written by Ted Unangst
 */

#include <string.h>
#include "explicit_bzero.h"

/*
 * explicit_bzero - don't let the compiler optimize away bzero
 */
void
compat_explicit_bzero(void *p, size_t n)
{
	bzero(p, n);
}
