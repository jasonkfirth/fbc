/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_file_seek.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevFileSeek( FB_FILE *handle, fb_off_t offset, int whence )
{
    int res;
    FILE *fp;

	FB_LOCK();

    fp = (FILE*) handle->opaque;

    if( fp == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
    }

	res = fb_ErrorSetNum( fseeko( fp, offset, whence ) == 0 ? FB_RTERROR_OK : FB_RTERROR_FILEIO );

	FB_UNLOCK();

	return res;
}

/* end of devices/dev_file_seek.c */
