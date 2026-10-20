/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_scrn_close.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevScrnClose( FB_FILE *handle )
{
    FB_LOCK();
	fb_DevScrnEnd( handle );
    FB_UNLOCK();
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of devices/dev_scrn_close.c */
