/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_file_write.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevFileWrite( FB_FILE *handle, const void* value, size_t valuelen )
{
    FILE *fp;

    FB_LOCK();

    fp = (FILE*) handle->opaque;

	if( fp == NULL ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	/* do write */
	if( FB_FWRITE_LARGE( value, valuelen, fp ) != valuelen ) {
		FB_UNLOCK();
		return fb_ErrorSetNum( FB_RTERROR_FILEIO );
	}

	FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of devices/dev_file_write.c */
