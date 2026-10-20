/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/file_datetime.c
 * Purpose: get file date/time by filename.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL double fb_FileDateTime( const char *filename )
{
	(void)filename;
	return 0.0;
}

/* end of drivers/xbox/file_datetime.c */
