/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/sys_getcwd.c
 * Purpose: get current dir.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

ssize_t fb_hGetCurrentDir( char *dst, ssize_t maxlen )
{
	if( fb_hGetExePath( dst, maxlen ) != NULL )
		return strlen( dst );

	*dst = '\0';
	return 0;
}

/* end of drivers/xbox/sys_getcwd.c */
