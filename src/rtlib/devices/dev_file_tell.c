/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_file_tell.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevFileTell( FB_FILE *handle, fb_off_t *pOffset )
{
	FILE *fp;

	FB_LOCK();

	fp = (FILE*) handle->opaque;

	if( fp == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	*pOffset = ftello( fp );

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of devices/dev_file_tell.c */
