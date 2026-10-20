/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL size_t fb_GetMemAvail( int mode )
{
	MM_STATISTICS ms;
	MmQueryStatistics(&ms);
	return ms.AvailablePages * 4096;
}

/* end of drivers/xbox/sys_fmem.c */
