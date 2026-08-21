/*
    FreeBASIC runtime library
    -------------------------

    File: aros/file_input_float.c

    Purpose:

        Read floating-point INPUT tokens accurately on AROS.

    Responsibilities:

        - preserve the shared integer and radix-prefixed conversion paths
        - route decimal tokens through the AROS FreeBASIC numeric parser
        - return the normal FreeBASIC runtime status

    This file intentionally does NOT contain:

        - file-token parsing
        - a second decimal conversion implementation
        - architecture-specific floating-point policy

    AROS stdc's strtod() does not always round decimal input correctly.  The
    platform string conversion replacement owns that workaround, so INPUT and
    VAL share one implementation and one set of syntax rules.
*/

#include "../fb.h"

/* ------------------------------------------------------------------------- */
/* FreeBASIC INPUT entry points                                              */
/* ------------------------------------------------------------------------- */

FBCALL int fb_InputSingle( float *destination )
{
	char buffer[FB_INPUT_MAXNUMERICLEN + 1];
	ssize_t length;
	int is_float;

	length = fb_FileInputNextToken( buffer, FB_INPUT_MAXNUMERICLEN,
		FB_FALSE, &is_float );

	if( is_float == FALSE )
	{
		if( length <= FB_INPUT_MAXINTLEN )
			*destination = (float)fb_hStr2Int( buffer, length );
		else if( length <= FB_INPUT_MAXLONGLEN )
			*destination = (float)fb_hStr2Longint( buffer, length );
		else if( buffer[0] == '&' )
			*destination = (float)fb_hStr2Longint( buffer, length );
		else
			*destination = (float)fb_hStr2Double( buffer, length );
	}
	else
	{
		*destination = (float)fb_hStr2Double( buffer, length );
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

FBCALL int fb_InputDouble( double *destination )
{
	char buffer[FB_INPUT_MAXNUMERICLEN + 1];
	ssize_t length;
	int is_float;

	length = fb_FileInputNextToken( buffer, FB_INPUT_MAXNUMERICLEN,
		FB_FALSE, &is_float );

	if( is_float == FALSE )
	{
		if( length <= FB_INPUT_MAXINTLEN )
			*destination = (double)fb_hStr2Int( buffer, length );
		else if( length <= FB_INPUT_MAXLONGLEN )
			*destination = (double)fb_hStr2Longint( buffer, length );
		else if( buffer[0] == '&' )
			*destination = (double)fb_hStr2Longint( buffer, length );
		else
			*destination = fb_hStr2Double( buffer, length );
	}
	else
	{
		*destination = fb_hStr2Double( buffer, length );
	}

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of aros/file_input_float.c */
