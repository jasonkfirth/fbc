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
	long pages = get_avphys_pages();
	long page_size = sysconf(_SC_PAGE_SIZE);
	(void)mode;
	if( pages <= 0 || page_size <= 0 )
		return 0;
	/* OS query failures are negative. Positive counts use the native unsigned
	   byte range, capped at its limit if the physical total cannot fit. */
	if( (size_t)pages > SIZE_MAX / (size_t)page_size )
		return SIZE_MAX;
	return (size_t)pages * (size_t)page_size;
}

/* end of drivers/linux/sys_fmem.c */
