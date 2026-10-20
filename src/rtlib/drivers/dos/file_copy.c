/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/file_copy.c
 * Purpose: file copy.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL int fb_FileCopy( const char *source, const char *destination )
{
	return fb_CrtFileCopy( source, destination );
}

/* end of drivers/dos/file_copy.c */
