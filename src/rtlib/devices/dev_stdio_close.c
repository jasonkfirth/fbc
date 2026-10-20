/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_stdio_close.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevStdIoClose( FB_FILE *handle )
{
	FB_LOCK();

	handle->opaque = NULL;

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of devices/dev_stdio_close.c */
