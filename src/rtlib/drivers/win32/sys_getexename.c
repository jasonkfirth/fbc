/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_getexename.c
 * Purpose: get the executable's name.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>
#include <stdint.h>

char *fb_hGetExeName( char *dst, ssize_t maxlen )
{
	if( (dst == NULL) || (maxlen <= 0) ||
	    ((uint64_t)maxlen > (uint64_t)MAXDWORD) )
		return NULL;

	GetModuleFileName( GetModuleHandle( NULL ), dst, (DWORD)maxlen );

	char *p = strrchr( dst, '\\' );
	if( p != NULL )
		++p;
	else
		p = dst;

	return p;
}

/* end of drivers/win32/sys_getexename.c */
