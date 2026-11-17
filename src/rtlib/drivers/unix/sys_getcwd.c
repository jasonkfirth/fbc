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

	/* A successful getcwd includes its terminator within the positive signed
	   capacity, so the resulting length remains representable as ssize_t. */
	if( getcwd( dst, (size_t)maxlen ) != NULL )
		return (ssize_t)strlen( dst );
	return 0;
}

/* end of drivers/unix/sys_getcwd.c */
