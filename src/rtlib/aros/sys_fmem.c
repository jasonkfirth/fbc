/*
    FreeBASIC runtime support for AROS
    ----------------------------------

    File: sys_fmem.c

    Purpose:

        Implement FRE() memory-availability reporting through exec.library.

    Responsibilities:

        - query the currently available public memory from the Exec allocator
        - convert the native pointer-width byte count to the runtime size type
        - preserve the historical FRE() signature on every AROS architecture

    This file intentionally does NOT contain:

        - heap allocator accounting
        - virtual-memory policy
        - architecture-specific memory limits

    AROS note:

        AvailMem(MEMF_ANY) returns the bytes currently available from all
        allocatable memory regions.  IPTR follows the native pointer width, so
        this remains valid for m68k, ARM, and x86_64 without an ABI override.
*/

#include "../fb.h"

#include <exec/memory.h>
#include <proto/exec.h>

/* ------------------------------------------------------------------------- */
/* Public memory query                                                       */
/* ------------------------------------------------------------------------- */

FBCALL size_t fb_GetMemAvail( int mode )
{
	(void)mode;

	return (size_t)AvailMem( MEMF_ANY );
}

/* end of sys_fmem.c */
