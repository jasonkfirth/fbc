/*
    FreeBASIC runtime support
    -------------------------

    File: sys_fmem.c

    Purpose:

        Implement fre() memory-availability reporting for Haiku.

    Responsibilities:

        • query the amount of free physical memory
        • provide the fb_GetMemAvail() runtime entry point

    This file intentionally does NOT contain:

        • heap allocator internals
        • virtual memory tuning
        • process-specific memory accounting

    Notes:

        Haiku exposes system memory information through the `system_info`
        structure. `free_memory` already reports a byte count, which is
        suitable for the historical fre() API used by the runtime.
*/

#ifndef DISABLE_HAIKU

#include "../fb.h"

#include <OS.h>

FBCALL size_t fb_GetMemAvail( int mode )
{
    system_info info;

    (void)mode;

    if (get_system_info(&info) != B_OK) {
        return 0;
    }

    if (info.free_memory < 0) {
        return 0;
    }

    return (size_t)info.free_memory;
}

#endif

/* end of sys_fmem.c */
