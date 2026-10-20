/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <windows.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
	MEMORYSTATUS ms;
	ms.dwLength = sizeof( MEMORYSTATUS );
	GlobalMemoryStatus( &ms );
	return ms.dwAvailPhys;
}

/* end of drivers/win32/sys_fmem.c */
