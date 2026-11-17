/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/sys_getshortpath.c
 * Purpose: Provide sys getshortpath runtime support.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

char *fb_hGetShortPath( char *src, char *dst, ssize_t maxlen )
{
	size_t copied = 0, limit;

	if( dst == NULL || maxlen <= 0 )
		return NULL;
	if( src == NULL ) {
		dst[0] = '\0';
		return dst;
	}

	/* DOS has no active LFN-to-SFN resolver in this runtime. */
	limit = (size_t)maxlen - 1;
	while( copied < limit && src[copied] != '\0' )
		copied++;
	memmove( dst, src, copied );
	dst[copied] = '\0';

	return dst;
}

/* end of drivers/dos/sys_getshortpath.c */
