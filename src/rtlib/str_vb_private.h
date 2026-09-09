/*
    FreeBASIC Runtime Library
    File: str_vb_private.h
    Purpose: Share the byte comparison and allocation limits of string.bi.
    Responsibilities: Deterministic ASCII folding and checked string sizing.
    This file does not contain locale state or public entry points.
*/

#ifndef FB_STR_VB_PRIVATE_H
#define FB_STR_VB_PRIVATE_H

/* fb_hStrRealloc rounds up to 32 bytes and reserves another 12.5 percent.
   Leave room for both operations and the terminator in signed ssize_t. */
#define FB_STR_VB_MAXLEN ((FB_STRSIZEMSK / 9) * 8 - 32)

static __inline__ unsigned char fb_hStrFoldASCII( unsigned char ch )
{
	if( ch >= 'A' && ch <= 'Z' )
		ch += 'a' - 'A';
	return ch;
}

static __inline__ int fb_hStrCompareBytes( const char *a, const char *b,
                                         ssize_t len, int compare )
{
	ssize_t i;

	if( compare == 0 )
		return memcmp( a, b, len );

	for( i = 0; i < len; ++i ) {
		unsigned char x = fb_hStrFoldASCII( (unsigned char)a[i] );
		unsigned char y = fb_hStrFoldASCII( (unsigned char)b[i] );
		if( x != y )
			return (x < y) ? -1 : 1;
	}
	return 0;
}

#endif

/* end of str_vb_private.h */
