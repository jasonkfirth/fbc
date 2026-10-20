/*
 * Project: FreeBASIC runtime library
 * File: files/file_input_ushort.c
 * Purpose: input function for usigned shorts.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <math.h>

FBCALL int fb_InputUshort( unsigned short *dst )
{
    char buffer[FB_INPUT_MAXNUMERICLEN+1];
	ssize_t len;
	int isfp;

	len = fb_FileInputNextToken( buffer, FB_INPUT_MAXNUMERICLEN, FB_FALSE, &isfp );

	if( isfp == FALSE )
		*dst = (unsigned short)fb_hStr2UInt( buffer, len );
	else
		*dst = (unsigned short)rint( fb_hStr2Double( buffer, len ) );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of files/file_input_ushort.c */
