/*
 * Project: FreeBASIC runtime library
 * File: files/file_putarray.c
 * Purpose: put # function for arrays.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_FilePutArray( int fnum, int pos, FBARRAY *src )
{
	return fb_FilePutData( fnum, pos, src->ptr, src->size, TRUE, FALSE );
}

FBCALL int fb_FilePutArrayLarge( int fnum, long long pos, FBARRAY *src )
{
	return fb_FilePutData( fnum, pos, src->ptr, src->size, TRUE, FALSE );
}

/* end of files/file_putarray.c */
