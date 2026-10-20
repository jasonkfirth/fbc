/*
 * Project: FreeBASIC runtime library
 * File: strings/wide/strw_comp.c
 * Purpose: wstring compare function.
 * Responsibilities: Wide string operations using the target FB_WCHAR width.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_WstrCompare( const FB_WCHAR *str1, const FB_WCHAR *str2 )
{
	int res;
	ssize_t str1_len, str2_len;

	/* both not null? */
	if( (str1 != NULL) && (str2 != NULL) )
	{
		str1_len = fb_wstr_Len( str1 );
        str2_len = fb_wstr_Len( str2 );

        res = fb_wstr_Compare( str1, str2, ((str1_len < str2_len) ? str1_len : str2_len) );
        if( (res == 0) && (str1_len != str2_len) )
        	res = (( str1_len > str2_len ) ? 1 : -1 );

        return res;
	}

	/* left null? */
	if( str1 == NULL )
	{
		/* right also null? return eq */
		if( (str2 == NULL) || (fb_wstr_Len( str2 ) == 0) )
			return 0;

		/* return lt */
		return -1;
	}

    /* only right is null. is left empty? return eq */
    if( fb_wstr_Len( str1 ) == 0 )
        return 0;

    /* return gt */
    return 1;
}

/* end of strings/wide/strw_comp.c */
