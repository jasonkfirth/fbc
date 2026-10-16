/*
    FreeBASIC runtime support for classic AmigaOS
    ---------------------------------------------
    File: sys_portio.c
    Purpose: Complete the runtime's INP and OUT hook contract.
    Responsibilities:
        - reject PC hardware port access on the native Amiga target
        - preserve the input and output error conventions used by hook_ports.c
    This file intentionally does NOT contain chipset memory access or PC emulation.

    Amiga devices use native drivers and memory-mapped registers. A PC port
    number cannot be used as a native register address. Graphics hooks may
    handle legacy VGA operations before these fallback functions are called.
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
