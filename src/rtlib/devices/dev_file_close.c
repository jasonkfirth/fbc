/*
 * Project: FreeBASIC runtime library
 * File: devices/dev_file_close.c
 * Purpose: file device.
 * Responsibilities: File-device callbacks and private stream state.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

int fb_DevFileClose( FB_FILE *handle )
{
    FILE *fp;

    FB_LOCK();

    fp = (FILE*) handle->opaque;

    if( fp != NULL ) {
        fclose( fp );
    }

	handle->opaque = NULL;

    FB_UNLOCK();

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of devices/dev_file_close.c */
