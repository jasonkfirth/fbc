/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/sys_getexepath.c
 * Purpose: get the executable path.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

char *fb_hGetExePath( char *dst, ssize_t maxlen )
{
	/* There is no executable, but return a valid path: / */
	if( dst == NULL || maxlen <= 0 )
		return NULL;
	if( maxlen == 1 ) {
		dst[0] = '\0';
		return NULL;
	}
	dst[0] = '/';
	dst[1] = '\0';
	return dst;
}

/* end of drivers/js/sys_getexepath.c */
