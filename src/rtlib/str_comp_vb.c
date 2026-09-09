/*
    FreeBASIC Runtime Library
    File: str_comp_vb.c
    Purpose: Implement the optional StrComp function.
    Responsibilities: Length-aware comparison and normalized -1/0/1 results.
    This file does not change intrinsic comparisons or implement VB collation.
*/

#include "fb.h"
#include "str_vb_private.h"

FBCALL int fb_StrComp( FBSTRING *str1, FBSTRING *str2, int compare )
{
	ssize_t len1, len2, len;
	int result = 0, error = FB_RTERROR_OK;

	/* As with other runtime string calls, the lock protects temporary
	   descriptors. Callers must synchronize writes to their own strings. */
	FB_STRLOCK();
	len1 = (str1 && str1->data) ? FB_STRSIZE( str1 ) : 0;
	len2 = (str2 && str2->data) ? FB_STRSIZE( str2 ) : 0;
	len = (len1 < len2) ? len1 : len2;
	if( compare != 0 && compare != 1 ) {
		error = FB_RTERROR_ILLEGALFUNCTIONCALL;
	} else {
		if( len > 0 )
			result = fb_hStrCompareBytes( str1->data, str2->data, len, compare );
		if( result == 0 )
			result = (len1 > len2) - (len1 < len2);
		else
			result = (result > 0) ? 1 : -1;
	}

	fb_hStrDelTemp_NoLock( str1 );
	if( str2 != str1 )
		fb_hStrDelTemp_NoLock( str2 );
	FB_STRUNLOCK();
	fb_ErrorSetNum( error );
	return result;
}

/* end of str_comp_vb.c */
