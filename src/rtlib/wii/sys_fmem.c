/*
    FreeBASIC runtime Wii memory availability
    -----------------------------------------

    File: sys_fmem.c

    Purpose:

        Report the memory currently available through libogc arenas.

    Responsibilities:

        - expose the FRE runtime entry point
        - combine the remaining MEM1 and MEM2 arena sizes without overflow

    This file intentionally does NOT contain:

        - heap allocation policy
        - virtual memory accounting
        - graphics memory accounting
*/

#include "../fb.h"
#include <ogc/system.h>

FBCALL size_t fb_GetMemAvail(int mode)
{
	(void)mode;

	return (size_t)SYS_GetArena1Size() + (size_t)SYS_GetArena2Size();
}

/* end of sys_fmem.c */
