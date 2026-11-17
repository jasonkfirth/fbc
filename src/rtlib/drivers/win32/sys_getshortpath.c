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
	if( (src == NULL) || (dst == NULL) || (maxlen <= 0) )
		return dst;

	if( strchr( src, 32 ) == NULL ) {
		strncpy( dst, src, (size_t)maxlen );
		dst[maxlen-1] = '\0';
	} else {
		size_t capacity_size = (size_t)maxlen;
		DWORD capacity = (capacity_size > (size_t)MAXDWORD) ? MAXDWORD : (DWORD)capacity_size;
		GetShortPathName( src, dst, capacity );
	}

	return dst;
}

/* end of drivers/win32/sys_getshortpath.c */
