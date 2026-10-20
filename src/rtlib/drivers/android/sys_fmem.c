/*
 * Project: FreeBASIC runtime library
 * File: drivers/android/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for android.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <unistd.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
	return sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
}

/* end of drivers/android/sys_fmem.c */
