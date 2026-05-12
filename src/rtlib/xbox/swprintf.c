/*
    Project: FreeBASIC Xbox Runtime
    --------------------------------

    File: xbox/swprintf.c

    Purpose:

        Provide the narrow-backed swprintf() entry points needed by the
        common WSTRING conversion runtime on nxdk.

    Responsibilities:

        - translate the ASCII format strings used by the runtime
        - delegate formatting to the target C library's vsnprintf()
        - copy the resulting narrow text into the destination wide buffer

    This file intentionally does NOT contain:

        - a full locale-aware wide stdio implementation
        - general Unicode formatting support
        - FreeBASIC WSTRING parsing or allocation logic
*/

#include "../fb.h"

/* ------------------------------------------------------------------------- */
/* Local helpers                                                             */
/* ------------------------------------------------------------------------- */

static int fb_xbox_wformat_to_zstring( char *dst, size_t dst_len, const wchar_t *src )
{
	size_t i;

	if( (dst == NULL) || (dst_len == 0) || (src == NULL) ) {
		return -1;
	}

	for( i = 0; i + 1 < dst_len; ++i ) {
		wchar_t ch = src[i];

		if( ch == L'\0' ) {
			dst[i] = '\0';
			return 0;
		}

		dst[i] = (ch <= 127) ? (char)ch : '?';
	}

	dst[dst_len - 1] = '\0';

	return -1;
}

static int fb_xbox_copy_zstring_to_wstring( wchar_t *dst, size_t dst_len, const char *src )
{
	size_t i;

	if( (dst == NULL) || (dst_len == 0) || (src == NULL) ) {
		return -1;
	}

	for( i = 0; i + 1 < dst_len; ++i ) {
		if( src[i] == '\0' ) {
			dst[i] = L'\0';
			return 0;
		}

		dst[i] = (unsigned char)src[i];
	}

	dst[dst_len - 1] = L'\0';

	return 0;
}

/* ------------------------------------------------------------------------- */
/* Public entry points                                                       */
/* ------------------------------------------------------------------------- */

int vswprintf( wchar_t *wcs, size_t maxlen, const wchar_t *format, va_list args )
{
	char fmt[64];
	char *buffer;
	int result;

	if( (wcs == NULL) || (maxlen == 0) || (format == NULL) ) {
		return -1;
	}

	if( fb_xbox_wformat_to_zstring( fmt, sizeof( fmt ), format ) != 0 ) {
		wcs[0] = L'\0';
		return -1;
	}

	buffer = (char *)malloc( maxlen );
	if( buffer == NULL ) {
		wcs[0] = L'\0';
		return -1;
	}

	result = vsnprintf( buffer, maxlen, fmt, args );
	if( result >= 0 ) {
		fb_xbox_copy_zstring_to_wstring( wcs, maxlen, buffer );
	} else {
		wcs[0] = L'\0';
	}

	free( buffer );

	return result;
}

int swprintf( wchar_t *wcs, size_t maxlen, const wchar_t *format, ... )
{
	va_list args;
	int result;

	va_start( args, format );
	result = vswprintf( wcs, maxlen, format, args );
	va_end( args );

	return result;
}

/* end of xbox/swprintf.c */
