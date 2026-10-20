/*
 * Project: FreeBASIC runtime library
 * File: files/file_encod.c
 * Purpose: string to file encoding function.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"
#include <strings.h>


/*:::::*/
FB_FILE_ENCOD fb_hFileStrToEncoding( const char *encoding )
{
	if( encoding == NULL )
		return FB_FILE_ENCOD_DEFAULT;

	if( strncasecmp( encoding, "UTF", 3 ) == 0 )
	{
		encoding += 3;

		if( *encoding == '-' )
			++encoding;

		if( *encoding == '8' )
			return FB_FILE_ENCOD_UTF8;

		if( strcmp( encoding, "16" ) == 0 )
			return FB_FILE_ENCOD_UTF16;

		if( strcmp( encoding, "32" ) == 0 )
			return FB_FILE_ENCOD_UTF32;
	}
	else
	{
		if( strncasecmp( encoding, "ASC", 3 ) == 0 )
			return FB_FILE_ENCOD_ASCII;
	}

	return FB_FILE_ENCOD_DEFAULT;
}

/* end of files/file_encod.c */
