/*
 * Project: FreeBASIC runtime library
 * File: drivers/netbsd/sys_fmem.c
 * Purpose: fre() function.
 * Responsibilities: Target-specific runtime support for netbsd.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <unistd.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
#if defined( _SC_AVPHYS_PAGES ) && defined( _SC_PAGESIZE )
	long pages = sysconf( _SC_AVPHYS_PAGES );
	long page_size = sysconf( _SC_PAGESIZE );

	if( (pages > 0) && (page_size > 0) )
		return (size_t)pages * (size_t)page_size;
#endif

	(void)mode;
	return 0;
}

/* end of drivers/netbsd/sys_fmem.c */
