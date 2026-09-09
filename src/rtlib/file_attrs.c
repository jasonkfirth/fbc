/*
    FreeBASIC Runtime Library
    File: file_attrs.c
    Purpose: Provide a checked fallback for pathname attribute operations.
    Responsibilities: Keep the API linkable on targets without an adapter.
    This file does not emulate attributes or report an unsupported write as OK.
*/

#include "fb.h"

FBCALL int fb_FileGetAttr( const char *filename )
{
	(void)filename;
	fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	return -1;
}

FBCALL int fb_FileSetAttr( const char *filename, int attributes )
{
	(void)filename;
	(void)attributes;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

/* end of file_attrs.c */
