/*
    FreeBASIC runtime library
    -------------------------

    File: aros/strw_convfrom.c

    Purpose:

        Convert FreeBASIC wide strings to double accurately on AROS.

    Responsibilities:

        - accept the numeric ASCII subset used by FreeBASIC VAL
        - share decimal and radix parsing with the AROS narrow-string path
        - avoid AROS wcstod(), which inherits the platform strtod rounding bug

    This file intentionally does NOT contain:

        - locale-sensitive wide-character conversion
        - a duplicate decimal parser
        - architecture-specific floating-point policy
*/

#include "../fb.h"

/* ------------------------------------------------------------------------- */
/* Wide numeric conversion                                                   */
/* ------------------------------------------------------------------------- */

FBCALL double fb_WstrToDouble( const FB_WCHAR *source, ssize_t length )
{
	char *narrow;
	double result;
	ssize_t index;

	if( (source == NULL) || (length <= 0) )
		return 0.0;

	narrow = malloc( (size_t)length + 1 );
	if( narrow == NULL )
		return 0.0;

	for( index = 0; index < length; ++index )
	{
		if( (unsigned long)source[index] > 0x7fUL )
		{
			narrow[index] = '\0';
			length = index;
			break;
		}
		narrow[index] = (char)source[index];
	}
	narrow[length] = '\0';

	result = fb_hStr2Double( narrow, length );
	free( narrow );
	return result;
}

FBCALL double fb_WstrVal( const FB_WCHAR *string )
{
	ssize_t length;

	if( string == NULL )
		return 0.0;

	length = fb_wstr_Len( string );
	if( length == 0 )
		return 0.0;
	return fb_WstrToDouble( string, length );
}

/* end of aros/strw_convfrom.c */
