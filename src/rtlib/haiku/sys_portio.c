/*
    FreeBASIC runtime support
    -------------------------

    File: sys_portio.c

    Purpose:

        Provide port I/O entry points for the Haiku target.

    Responsibilities:

        • satisfy the fb_hIn() and fb_hOut() runtime symbols
        • reject unsupported hardware port access cleanly

    This file intentionally does NOT contain:

        • x86 inb/outb instructions
        • privilege elevation
        • platform-specific device drivers

    Notes:

        Direct port I/O is highly platform-specific and generally not
        available to hosted user-space code on Haiku. Returning the
        standard illegal-function error keeps behaviour explicit and
        avoids undefined low-level access paths.
*/

#ifndef DISABLE_HAIKU

#include "../fb.h"

int fb_hIn( unsigned short port )
{
    (void)port;

    return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_hOut( unsigned short port, unsigned char value )
{
    (void)port;
    (void)value;

    return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

#endif

/* end of sys_portio.c */
