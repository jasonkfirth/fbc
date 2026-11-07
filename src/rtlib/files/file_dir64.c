/*
 * Project: FreeBASIC runtime library
 * File: files/file_dir64.c
 * Purpose: Provide file dir64 runtime support.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING_ACCESS *fb_Dir64( FBSTRING_ACCESS *filespec, int attrib, long long *outattrib )
{
	int ioutattrib;
	FBSTRING_ACCESS *res;

	res = fb_Dir( filespec, attrib, &ioutattrib );

	*outattrib = ioutattrib;
	return res;
}

/* end of files/file_dir64.c */
