/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/sys_getcwd.c
 * Purpose: get current dir.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

ssize_t fb_hGetCurrentDir( char *dst, ssize_t maxlen )
{
	if( (dst == NULL) || (maxlen <= 0) )
		return 0;

	if( getcwd( dst, maxlen ) != NULL )
		return strlen( dst );
	return 0;
}

/* end of drivers/unix/sys_getcwd.c */
