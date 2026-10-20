/*
 * Project: FreeBASIC runtime library
 * File: files/file_dirnext.c
 * Purpose: dir().
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL FBSTRING *fb_DirNext( int *attrib )
{
	static FBSTRING fname = { 0, 0, 0 };
	return fb_Dir( &fname, 0, attrib );
}

/* end of files/file_dirnext.c */
