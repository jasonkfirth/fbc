/*
    FreeBASIC runtime support for RISC OS
    -------------------------------------

    File: sys_portio.c

    Purpose:

        Provide the INP() and OUT runtime entry points for RISC OS.

    Responsibilities:

        - reject unsupported PC-style hardware port access
        - return values with the error convention expected by hook_ports.c

    This file intentionally does NOT contain:

        - processor-specific port instructions
        - privilege changes
        - RISC OS device-driver access

    Runtime contract:

        fb_hIn() returns a negative error so hook_ports.c can distinguish it
        from a byte value.  fb_hOut() returns the normal positive runtime error
        number.  The difference is part of the existing port-I/O hook API.
*/

#include "../fb.h"

int fb_hIn( unsigned short port )
{
	(void)port;

	return -fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_hOut( unsigned short port, unsigned char value )
{
	(void)port;
	(void)value;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of sys_portio.c */
