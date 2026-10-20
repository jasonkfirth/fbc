/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/sys_getexename.c
 * Purpose: get the executable's name.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <string.h>

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	const char *name = "D:\\default.xbe";
	ssize_t len = strlen( name );

	if( maxlen <= 0 )
		return dst;

	if( len >= maxlen )
		len = maxlen - 1;

	memcpy( dst, name, len );
	dst[len] = '\0';

	return dst;
}

/* end of drivers/xbox/sys_getexename.c */
