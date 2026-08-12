/*
    FreeBASIC runtime support for RISC OS
    -------------------------------------

    File: sys_fmem.c

    Purpose:

        Implement FRE() memory-availability reporting for RISC OS.

    Responsibilities:

        - query the Wimp free-memory pool through Wimp_SlotSize
        - validate the SWI result before converting it to size_t
        - return a defined value when the Wimp is unavailable

    This file intentionally does NOT contain:

        - heap allocator accounting
        - application slot resizing
        - Wimp task initialization

    Wimp_SlotSize contract:

        R0 = -1 and R1 = -1 request a read-only query.  On success, R2 is the
        number of bytes in the Wimp free pool.  CLI programs may run without
        the Wimp, in which case the SWI reports an error and FRE() returns zero.
*/

#include "../fb.h"

#include <kernel.h>
#include <swis.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
	_kernel_swi_regs regs = { { 0 } };

	(void)mode;

	regs.r[0] = -1;
	regs.r[1] = -1;

	if( _kernel_swi( Wimp_SlotSize, &regs, &regs ) != NULL )
		return 0;

	if( regs.r[2] <= 0 )
		return 0;

	return (size_t)regs.r[2];
}

/* end of sys_fmem.c */
