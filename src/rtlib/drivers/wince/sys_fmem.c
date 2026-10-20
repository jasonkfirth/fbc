/*
 * Project: FreeBASIC runtime library
 * File: drivers/wince/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for wince.
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

/* end of drivers/wince/sys_fmem.c */
