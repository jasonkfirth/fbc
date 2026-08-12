/*
    DOS wide-string ASC implementation

    This complete target file is selected by basename precedence.  Keep DOS
    behavior here instead of adding HOST_DOS branches to the shared source.
*/

/* ascw function */

#include "fb.h"

FBCALL unsigned int fb_WstrAsc( const FB_WCHAR *str, ssize_t pos )
{
	ssize_t len;

	if( str == NULL )
		return 0;

	len = fb_wstr_Len( str );
	if( (len == 0) || (pos <= 0) || (pos > len) )
		return 0;
	else
	/* on DOS, FB_WCHAR is a 'char' which is
	   typically signed.  To avoid an undesired
	   sign extension for chars >= 128, cast
	   to unsigned char first
	*/
	return (unsigned char)str[pos-1];
}
