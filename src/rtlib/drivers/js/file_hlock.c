/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/file_hlock.c
 * Purpose: low-level lock and unlock functions.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hFileLock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

int fb_hFileUnlock( FILE *f, fb_off_t inipos, fb_off_t size )
{
	return fb_ErrorSetNum( FB_RTERROR_FILEIO );
}

/* end of drivers/js/file_hlock.c */
