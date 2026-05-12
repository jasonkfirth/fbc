/*  valbool (string) function (boolean)  */

#include "../fb.h"

static int hStrEqAsciiNoCase( const char *src, ssize_t len, const char *text )
{
	ssize_t i;

	for( i = 0; i < len; i++ ) {
		char a = src[i];
		char b = text[i];

		if( b == '\0' )
			return 0;

		if( (a >= 'A') && (a <= 'Z') )
			a += 'a' - 'A';

		if( (b >= 'A') && (b <= 'Z') )
			b += 'a' - 'A';

		if( a != b )
			return 0;
	}

	return text[i] == '\0';
}

/** convert string to boolean value
 *  
 * return value must be 0|1
 *
 */
FBCALL char fb_hStr2Bool( char *src, ssize_t len )
{
	double val;

	if( hStrEqAsciiNoCase( src, len, fb_hBoolToStr( FALSE ) ) )
		return 0;

	if( hStrEqAsciiNoCase( src, len, fb_hBoolToStr( TRUE ) ) )
		return 1;

	val = fb_hStr2Double( src, len );

	if( (val != (double)(0.0) ) && (val != (double)(-0.0)) )
		return 1;

	return 0;
}

/*:::::*/
FBCALL char fb_VALBOOL ( FBSTRING *str )
{
    int	val;

	if( str == NULL )
	    return 0;

	if( (str->data == NULL) || (FB_STRSIZE( str ) == 0) )
		val = 0;
	else
		val = fb_hStr2Bool( str->data, FB_STRSIZE( str ) );

	/* del if temp */
	fb_hStrDelTemp( str );

	return val;
}
