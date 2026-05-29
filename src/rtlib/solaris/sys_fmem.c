/*
    FreeBASIC runtime library
    -------------------------

    File: sys_fmem.c

    Purpose:

        Implement the FRE runtime entry point on Solaris and illumos.

    Responsibilities:

        * report available physical memory through sysconf()
        * return a conservative zero value if the host cannot report it

    This file intentionally does NOT contain:

        * heap allocator accounting
        * process-specific memory limit handling
*/

#include "../fb.h"

#include <stdint.h>
#include <unistd.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
#if defined( _SC_AVPHYS_PAGES ) && defined( _SC_PAGESIZE )
	long pages;
	long page_size;

	pages = sysconf( _SC_AVPHYS_PAGES );
	page_size = sysconf( _SC_PAGESIZE );

	if( (pages > 0) && (page_size > 0) )
	{
		if( (unsigned long)pages > (SIZE_MAX / (unsigned long)page_size) )
			return SIZE_MAX;

		return (size_t)pages * (size_t)page_size;
	}
#endif

	(void)mode;
	return 0;
}

/* end of sys_fmem.c */
