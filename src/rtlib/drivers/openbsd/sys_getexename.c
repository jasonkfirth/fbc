/*
 * Project: FreeBASIC runtime library
 * File: drivers/openbsd/sys_getexename.c
 * Purpose: get the executable's name.
 * Responsibilities: Target-specific runtime support for openbsd.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	const char *p;
	size_t capacity;
	if( dst == NULL || maxlen <= 0 || fb_ctx.argv == NULL || fb_ctx.argv[0] == NULL )
		return NULL;

	capacity = (size_t)maxlen;
	p = strrchr( fb_ctx.argv[0], '/' );
	if( p )
		strlcpy( dst, p + 1, capacity );
	else
		strlcpy( dst, fb_ctx.argv[0], capacity );
	return dst;
}

/* end of drivers/openbsd/sys_getexename.c */
