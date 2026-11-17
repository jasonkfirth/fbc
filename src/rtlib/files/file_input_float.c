/*
 * Project: FreeBASIC runtime library
 * File: files/file_input_float.c
 * Purpose: input function for float's and double's.
 * Responsibilities: BASIC file operations and device-independent handle policy.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_InputSingle( float *dst )
{
    char buffer[FB_INPUT_MAXNUMERICLEN+1];
	ssize_t len;
	int isfp;
	long long parsed_value;

	len = fb_FileInputNextToken( buffer, FB_INPUT_MAXNUMERICLEN, FB_FALSE, &isfp );

	if( isfp == FALSE )
	{
		if( len <= FB_INPUT_MAXINTLEN )
		{
			parsed_value = fb_hStr2Int( buffer, len );
			*dst = (float)parsed_value;
		}
		else if( len <= FB_INPUT_MAXLONGLEN )
		{
			parsed_value = fb_hStr2Longint( buffer, len );
			*dst = (float)parsed_value;
		}
		else
		{
			if( buffer[0] == '&' )
			{
				parsed_value = fb_hStr2Longint( buffer, len );
				*dst = (float)parsed_value;
			}
			else
				*dst = strtof( buffer, NULL );
		}
	}
	else
		*dst = strtof( buffer, NULL );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

FBCALL int fb_InputDouble( double *dst )
{
    char buffer[FB_INPUT_MAXNUMERICLEN+1];
	ssize_t len;
	int isfp;
	long long parsed_value;

	len = fb_FileInputNextToken( buffer, FB_INPUT_MAXNUMERICLEN, FB_FALSE, &isfp );

	if( isfp == FALSE )
	{
		if( len <= FB_INPUT_MAXINTLEN )
		{
			parsed_value = fb_hStr2Int( buffer, len );
			*dst = (double)parsed_value;
		}
		else if( len <= FB_INPUT_MAXLONGLEN )
		{
			parsed_value = fb_hStr2Longint( buffer, len );
			*dst = (double)parsed_value;
		}
		else
		{
			if( buffer[0] == '&' )
			{
				parsed_value = fb_hStr2Longint( buffer, len );
				*dst = (double)parsed_value;
			}
			else
				*dst = strtod( buffer, NULL );
		}
	}
	else
		*dst = strtod( buffer, NULL );

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of files/file_input_float.c */
