/*
 * Project: FreeBASIC runtime library
 * File: console/io_setpos.c
 * Purpose: Sets a file handles line length.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_SetPos( FB_FILE *handle, int line_length )
{
    FB_LOCK();
    handle->line_length = line_length;
	FB_UNLOCK();

    return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of console/io_setpos.c */
