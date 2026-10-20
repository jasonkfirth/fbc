/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/file_copy.c
 * Purpose: file copy.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL int fb_FileCopy( const char *source, const char *destination )
{
	return fb_CrtFileCopy( source, destination );
}

/* end of drivers/xbox/file_copy.c */
