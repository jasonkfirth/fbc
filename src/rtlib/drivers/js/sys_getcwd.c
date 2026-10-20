/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/sys_getcwd.c
 * Purpose: get current dir.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

ssize_t fb_hGetCurrentDir( char *dst, ssize_t maxlen )
{
	if( getcwd( dst, maxlen ) != NULL )
		return strlen( dst );
	return 0;
}

/* end of drivers/js/sys_getcwd.c */
