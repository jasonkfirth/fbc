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
	const char *p = strrchr( __fb_ctx.argv[0], '/' );
	if( p )
		strlcpy( dst, p + 1, maxlen );
	else
		strlcpy( dst, __fb_ctx.argv[0], maxlen );
	return dst;
}

/* end of drivers/openbsd/sys_getexename.c */
