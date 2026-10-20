/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/file_hreset.c
 * Purpose: Provide file hreset runtime support.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

int fb_hFileResetEx( int streamno )
{
	return FALSE;
}

/* end of drivers/xbox/file_hreset.c */
