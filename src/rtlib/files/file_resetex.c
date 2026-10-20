/*
 * Project: FreeBASIC runtime library
 * File: files/file_resetex.c
 * Purpose: recover stdio after redirection.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/* streamno:
   0   Reset stdin
   1   Reset stdout */
FBCALL void fb_FileResetEx( int streamno )
{
	int errnum;

	FB_LOCK();

	switch( streamno ) {
	case 0:
	case 1:
		errnum = fb_hFileResetEx( streamno ) ? FB_RTERROR_OK : FB_RTERROR_FILEIO;
		break;
	default:
		errnum = FB_RTERROR_ILLEGALFUNCTIONCALL;
	}

	FB_UNLOCK();

	fb_ErrorSetNum( errnum );
}

/* end of files/file_resetex.c */
