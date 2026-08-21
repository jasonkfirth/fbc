/*
    FreeBASIC runtime library
    -------------------------

    File: aros/strw_convfrom_int.c

    Purpose:

        Convert FreeBASIC wide strings to signed integers on AROS.

    Responsibilities:

        - narrow the numeric ASCII subset into a bounded temporary buffer
        - share overflow and radix behavior with the narrow-string runtime
        - expose the normal VALINT wide-string entry point

    This file intentionally does NOT contain:

        - locale-sensitive wide-character conversion
        - a duplicate integer parser
        - non-AROS policy
*/

#include "../fb.h"

/* ------------------------------------------------------------------------- */
/* Signed integer conversion                                                 */
/* ------------------------------------------------------------------------- */

FBCALL int fb_WstrToInt( const FB_WCHAR *source, ssize_t length )
{
	char *narrow;
	int result;
	ssize_t index;

	if( (source == NULL) || (length <= 0) )
		return 0;

	narrow = malloc( (size_t)length + 1 );
	if( narrow == NULL )
		return 0;
	for( index = 0; index < length; ++index )
	{
		if( (unsigned long)source[index] > 0x7fUL )
		{
			length = index;
			break;
		}
		narrow[index] = (char)source[index];
	}
	narrow[length] = '\0';

	result = fb_hStr2Int( narrow, length );
	free( narrow );
	return result;
}

FBCALL int fb_WstrValInt( const FB_WCHAR *string )
{
	if( string == NULL )
		return 0;
	return fb_WstrToInt( string, fb_wstr_Len( string ) );
}

/* end of aros/strw_convfrom_int.c */
