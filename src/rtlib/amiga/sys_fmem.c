/*
    FreeBASIC runtime support for AmigaOS
    ----------------------------------

    File: sys_fmem.c

    Purpose:

        Implement FRE() memory-availability reporting through exec.library.

    Responsibilities:

        - query the currently available public memory from the Exec allocator
        - convert the native pointer-width byte count to the runtime size type
        - preserve the historical FRE() signature on every AmigaOS architecture

    This file intentionally does NOT contain:

        - heap allocator accounting
        - virtual-memory policy
        - architecture-specific memory limits

    AmigaOS note:

        AvailMem(MEMF_ANY) returns the total available memory. MEMF_LARGEST
        instead asks for the largest allocation the Exec allocator can satisfy.
*/

#include "../fb.h"

#include <exec/memory.h>
#include <proto/exec.h>

/* ------------------------------------------------------------------------- */
/* Public memory query                                                       */
/* ------------------------------------------------------------------------- */

FBCALL size_t fb_GetMemAvail( int mode )
{
	if( mode == 0 || mode == 1 )
		return (size_t)AvailMem( MEMF_ANY | MEMF_LARGEST );
	return (size_t)AvailMem( MEMF_ANY );
}

/* end of sys_fmem.c */
