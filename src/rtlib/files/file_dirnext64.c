/*
 * Project: FreeBASIC runtime library
 * File: files/file_dirnext64.c
 * Purpose: Provide file dirnext64 runtime support.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING_ACCESS *fb_DirNext64( long long *outattrib )
{
	int ioutattrib;
	FBSTRING_ACCESS *res;

	res = fb_DirNext( &ioutattrib );

	*outattrib = ioutattrib;
	return res;
}

/* end of files/file_dirnext64.c */
