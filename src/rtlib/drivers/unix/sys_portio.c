/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/sys_portio.c
 * Purpose: Provide the hosted Unix fallback for PC-style port I/O.
 * Responsibilities: Report unsupported hardware port access cleanly.
 * This file intentionally does NOT contain: Native port instructions or
 * platform-specific device access.
 */
#include "../../fb.h"

int fb_hIn( unsigned short port )
{
	(void)port;

	/* Graphics hooks handle emulated ports before this hosted fallback. */
	return -fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_hOut( unsigned short port, unsigned char value )
{
	(void)port;
	(void)value;

	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of drivers/unix/sys_portio.c */
