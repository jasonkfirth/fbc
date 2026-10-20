/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_getshortpath.c
 * Purpose: Provide sys getshortpath runtime support.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>

char *fb_hGetShortPath( char *src, char *dst, ssize_t maxlen )
{
	if( strchr( src, 32 ) == NULL ) {
		strncpy( dst, src, maxlen );
		dst[maxlen-1] = '\0';
	} else {
	 	GetShortPathName( src, dst, maxlen );
	}

	return dst;
}

/* end of drivers/win32/sys_getshortpath.c */
