/*
 * Project: FreeBASIC runtime library
 * File: drivers/xbox/file_input_float.c
 * Purpose: input function for float's and double's.
 * Responsibilities: Target-specific runtime support for xbox.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"

FBCALL int fb_InputSingle( float *dst )
{
    char buffer[FB_INPUT_MAXNUMERICLEN+1];
	ssize_t len;
	int isfp;

	len = fb_FileInputNextToken( buffer, FB_INPUT_MAXNUMERICLEN, FB_FALSE, &isfp );

	if( isfp == FALSE )
	{
		if( len <= FB_INPUT_MAXINTLEN )
			*dst = (float)fb_hStr2Int( buffer, len );
		else if( len <= FB_INPUT_MAXLONGLEN )
			*dst = (float)fb_hStr2Longint( buffer, len );
		else
		{
			if( buffer[0] == '&' )
				*dst = (float)fb_hStr2Longint( buffer, len );
			else
#ifdef HOST_XBOX
				*dst = (float)fb_hStr2Double( buffer, len );
#else
				*dst = strtof( buffer, NULL );
#endif
		}
	}
	else
#ifdef HOST_XBOX
		*dst = (float)fb_hStr2Double( buffer, len );
#else
		*dst = strtof( buffer, NULL );
#endif

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

FBCALL int fb_InputDouble( double *dst )
{
    char buffer[FB_INPUT_MAXNUMERICLEN+1];
	ssize_t len;
	int isfp;

	len = fb_FileInputNextToken( buffer, FB_INPUT_MAXNUMERICLEN, FB_FALSE, &isfp );

	if( isfp == FALSE )
	{
		if( len <= FB_INPUT_MAXINTLEN )
			*dst = (double)fb_hStr2Int( buffer, len );
		else if( len <= FB_INPUT_MAXLONGLEN )
			*dst = (double)fb_hStr2Longint( buffer, len );
		else
		{
			if( buffer[0] == '&' )
				*dst = (double)fb_hStr2Longint( buffer, len );
			else
#ifdef HOST_XBOX
				*dst = fb_hStr2Double( buffer, len );
#else
				*dst = strtod( buffer, NULL );
#endif
		}
	}
	else
#ifdef HOST_XBOX
		*dst = fb_hStr2Double( buffer, len );
#else
		*dst = strtod( buffer, NULL );
#endif

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of drivers/xbox/file_input_float.c */
