/*
    FreeBASIC Windows CE runtime
    ----------------------------

    File: wince/strw_convto_flt.c

    Purpose:

        Convert floating-point values to WSTRING without using the broken
        Windows CE %g formatter.

    Responsibilities:

        - reuse the target's accurate narrow significant-digit formatter
        - widen its ASCII numeric result into a temporary WSTRING
        - preserve FreeBASIC's trailing-decimal-point behavior

    This file intentionally does NOT contain:

        - the significant-digit formatting algorithm
        - localized decimal separators
        - QB-compatible leading-space conversion
*/

#include "../fb.h"

static FB_WCHAR *hFloatToWstr( double value, int digits )
{
	char text[16 + 8 + 1];
	FB_WCHAR *result;
	size_t length;
	size_t index;

	fb_hFloat2Str( value, text, digits, 0 );
	length = strlen( text );
	if( (length > 0) && (text[length - 1] == '.') ) {
		text[--length] = '\0';
	}

	result = fb_wstr_AllocTemp( (ssize_t)length );
	if( result == NULL ) {
		return NULL;
	}

	for( index = 0; index < length; index++ ) {
		result[index] = (FB_WCHAR)(unsigned char)text[index];
	}
	result[length] = _LC('\0');

	return result;
}

FBCALL FB_WCHAR *fb_FloatToWstr( float value )
{
	return hFloatToWstr( (double)value, 7 );
}

FBCALL FB_WCHAR *fb_DoubleToWstr( double value )
{
	return hFloatToWstr( value, 16 );
}

/* end of wince/strw_convto_flt.c */
