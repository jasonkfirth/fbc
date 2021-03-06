/* ucasew$ function */

#include "fb.h"

FBCALL FB_WCHAR *fb_WstrUcase2( const FB_WCHAR *src, int mode )
{
	FB_WCHAR *dst, *d;
	const FB_WCHAR *s;
	FB_WCHAR c;
	int chars, i;

	if( src == NULL )
		return NULL;

	chars = fb_wstr_Len( src );

	/* alloc temp string */
	dst = fb_wstr_AllocTemp( chars );
	if( dst == NULL )
		return NULL;

	s = src;
	d = dst;

	if( mode == 1 ) {
		for( i = 0; i < chars; i++ ) {
			c = *s++;
			if( (c >= 97) && (c <= 122) )
				c -= 97 - 65;
			*d++ = c;
		}
	} else {
		for( i = 0; i < chars; i++ ) {
			c = *s++;
			if( fb_wstr_IsLower( c ) )
				c = fb_wstr_ToUpper( c );
			*d++ = c;
		}
	}

	/* null char */
	*d = _LC('\0');

	return dst;
}
