/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_getexename.c
 * Purpose: get the executable's name.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	if( dst == NULL || maxlen <= 0 || fb_ctx.argv == NULL || fb_ctx.argv[0] == NULL )
		return NULL;
	strncpy(dst, fb_ctx.argv[0], (size_t)maxlen);
	dst[maxlen - 1] = '\0';
	char *p = strrchr( dst, '\\' );
	if( p != NULL )
		++p;
	else
		p = dst;
	return p;
}

/* end of drivers/dos/sys_getexename.c */
