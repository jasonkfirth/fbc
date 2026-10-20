/*
 * Project: FreeBASIC runtime library
 * File: drivers/android/sys_portio.c
 * Purpose: ports I/O functions.
 * Responsibilities: Target-specific runtime support for android.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hIn( unsigned short port )
{
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_hOut( unsigned short port, unsigned char value )
{
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of drivers/android/sys_portio.c */
