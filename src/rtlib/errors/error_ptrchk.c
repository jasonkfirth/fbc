/*
 * Project: FreeBASIC runtime library
 * File: errors/error_ptrchk.c
 * Purpose: null pointer checking function.
 * Responsibilities: Runtime errors, assertions and fault reporting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

/*:::::*/
FBCALL void *fb_NullPtrChk( void *ptr, int linenum, const char *fname )
{
	if( ptr == NULL )
		return (void *)fb_ErrorThrowEx( FB_RTERROR_NULLPTR, linenum, fname, NULL, NULL );
	else
		return NULL;
}

/* end of errors/error_ptrchk.c */
