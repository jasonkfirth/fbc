/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_getcwd.c
 * Purpose: get current dir.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>
#include <stdint.h>

ssize_t fb_hGetCurrentDir( char *dst, ssize_t maxlen )
{
	DWORD length;

	if( (dst == NULL) || (maxlen <= 0) ||
	    ((uint64_t)maxlen > (uint64_t)MAXDWORD) )
		return 0;

	length = GetCurrentDirectory( (DWORD)maxlen, dst );
#if PTRDIFF_MAX < UINT32_MAX
	if( length > (DWORD)PTRDIFF_MAX )
		return -1;
#endif

	return (ssize_t)length;
}

/* end of drivers/win32/sys_getcwd.c */
