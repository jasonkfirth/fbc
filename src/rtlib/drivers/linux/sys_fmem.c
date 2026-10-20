/*
 * Project: FreeBASIC runtime library
 * File: drivers/linux/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for linux.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/sysinfo.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
	return get_avphys_pages() * sysconf(_SC_PAGE_SIZE);
}

/* end of drivers/linux/sys_fmem.c */
