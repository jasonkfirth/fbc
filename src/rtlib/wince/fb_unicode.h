/*
    FreeBASIC Runtime Library
    -------------------------

    File: wince/fb_unicode.h

    Purpose:

        Select Unicode conversion primitives supported by the CeGCC Windows
        CE runtime.

    Responsibilities:

        - import the platform-neutral Unicode layouts and helpers
        - replace unavailable and broken 64-bit wide conversion functions
        - retain bounded wide-character formatting

    This file intentionally does NOT contain:

        - UTF-8 decoding algorithms
        - locale discovery
        - path conversion policy
*/

#ifndef __FB_WINCE_UNICODE_H__
#define __FB_WINCE_UNICODE_H__

#include "../fb_unicode.h"

/*
    CeGCC exposes _snwprintf() but omits the desktop _i64tow() and
    _ui64tow() helpers. The Windows CE 5 Coredll formatter also consumes only
    the low 32 bits for %lld and %llu, so integer conversion must not pass
    through the platform formatter.
*/
static __inline__ int fb_wince_WstrFromUInt64( FB_WCHAR *buffer,
                                              size_t buffer_chars,
                                              unsigned long long value,
                                              unsigned int radix )
{
	FB_WCHAR reversed[sizeof( unsigned long long ) * 8 + 1];
	size_t digits = 0;
	size_t output = 0;

	if( (buffer == NULL) || (buffer_chars == 0) ||
	    ((radix != 8) && (radix != 10)) ) {
		return -1;
	}

	do {
		reversed[digits++] = (FB_WCHAR)(_LC('0') + (value % radix));
		value /= radix;
	} while( (value != 0) && (digits < sizeof( reversed ) / sizeof( reversed[0] )) );

	while( (digits > 0) && (output + 1 < buffer_chars) ) {
		buffer[output++] = reversed[--digits];
	}
	buffer[output] = _LC('\0');

	return (digits == 0) ? (int)output : -1;
}

static __inline__ int fb_wince_WstrFromInt64( FB_WCHAR *buffer,
                                             size_t buffer_chars,
                                             long long value )
{
	unsigned long long magnitude;
	int result;

	if( value >= 0 ) {
		return fb_wince_WstrFromUInt64( buffer, buffer_chars,
		                                 (unsigned long long)value, 10 );
	}
	if( (buffer == NULL) || (buffer_chars < 2) ) {
		return -1;
	}

	buffer[0] = _LC('-');
	magnitude = (unsigned long long)(-(value + 1));
	magnitude += 1;
	result = fb_wince_WstrFromUInt64( buffer + 1, buffer_chars - 1,
	                                  magnitude, 10 );

	return (result < 0) ? -1 : result + 1;
}

#undef FB_WSTR_FROM_INT64
#define FB_WSTR_FROM_INT64( buffer, num ) \
	fb_wince_WstrFromInt64( buffer, sizeof( long long ) * 3 + 1, \
	                        (long long)(num) )

#undef FB_WSTR_FROM_UINT64
#define FB_WSTR_FROM_UINT64( buffer, num ) \
	fb_wince_WstrFromUInt64( buffer, sizeof( unsigned long long ) * 3 + 1, \
	                         (unsigned long long)(num), 10 )

#undef FB_WSTR_FROM_UINT64_OCT
#define FB_WSTR_FROM_UINT64_OCT( buffer, num ) \
	fb_wince_WstrFromUInt64( buffer, sizeof( unsigned long long ) * 4 + 1, \
	                         (unsigned long long)(num), 8 )

#endif

/* end of wince/fb_unicode.h */
