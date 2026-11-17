/*
    Project: FreeBASIC Runtime Library support for Windows CE
    ------------------------------------------------

    File: drivers/wince/intl_core.c

    Purpose:

        Implement character-set conversion and locale-string retrieval for
        the Unicode-only Windows CE API surface.

    Responsibilities:

        - convert temporary FreeBASIC strings through UTF-16
        - query locale data with GetLocaleInfoW
        - expose locale values as narrow strings expected by the shared API
        - support caller-owned and runtime-allocated result buffers

    This file intentionally does NOT contain: - date or time format parsing
        - locale selection policy
        - desktop Windows ANSI API assumptions
*/

#include "../../fb.h"
#include "fb_private_intl.h"

#include <limits.h>

/* ------------------------------------------------------------------------- */
/* Character-set conversion                                                  */
/* ------------------------------------------------------------------------- */

static FBSTRING_ACCESS *fb_hIntlConvertToWC( FBSTRING_ACCESS *source, UINT source_cp )
{
	FBSTRING_ACCESS *res = &fb_ctx.null_desc;
	WCHAR *wide_buffer = NULL;
	ssize_t source_size;
	size_t wide_bytes;
	int source_length;
	int chars_required;
	int chars_converted;
	size_t wide_count;

	if( source == NULL )
		return res;

    FB_STRLOCK();
	source_size = FB_STRSIZE( source );
	if( (source_size < 0) || (source_size > INT_MAX) )
		goto done;
	source_length = (int)source_size;

    chars_required = MultiByteToWideChar( source_cp,
                                          0,
                                          source->data,
                                          source_length,
                                          NULL,
                                          0 );
	if( (source_length > 0) && (chars_required <= 0) )
		goto done;
	wide_count = (size_t)chars_required + 1u;
	if( (wide_count > (SIZE_MAX / sizeof( WCHAR ))) ||
	    ((wide_count * sizeof( WCHAR ) - 1u) >
	     (size_t)FB_STRSIZEMSK) )
		goto done;
	wide_bytes = wide_count * sizeof( WCHAR );
	wide_buffer = (WCHAR *)malloc( wide_bytes );
	if( wide_buffer == NULL )
		goto done;
	if( chars_required > 0 ) {
		chars_converted = MultiByteToWideChar( source_cp, 0, source->data,
		                                      source_length, wide_buffer,
		                                      chars_required );
		if( chars_converted != chars_required )
			goto done;
	}
	wide_buffer[chars_required] = 0;
	res = fb_hStrAllocTemp_NoLock( NULL, (ssize_t)(wide_bytes - 1u) );
	if( res == NULL )
		res = &fb_ctx.null_desc;
	else
		memcpy( res->data, wide_buffer, wide_bytes );

done:
	free( wide_buffer );
    fb_hStrDelTemp_NoLock( source );
    FB_STRUNLOCK();

    return res;
}

static FBSTRING_ACCESS *fb_hIntlConvertFromWC( FBSTRING_ACCESS *source, UINT dest_cp )
{
	FBSTRING_ACCESS *res = &fb_ctx.null_desc;
	WCHAR *wide_source = NULL;
	ssize_t source_size;
	size_t wide_bytes;
	int source_chars;
    int chars_required;
	int chars_converted;

	if( source == NULL )
		return res;

    FB_STRLOCK();
	source_size = FB_STRSIZE( source );
	if( (source_size < 0) ||
	    ((source_size % (ssize_t)sizeof( WCHAR )) != 0) ||
	    ((size_t)(source_size / (ssize_t)sizeof( WCHAR )) > INT_MAX) )
		goto done;
	source_chars = (int)(source_size / (ssize_t)sizeof( WCHAR ));
	wide_bytes = (size_t)(source_chars + 1) * sizeof( WCHAR );
	wide_source = (WCHAR *)malloc( wide_bytes );
	if( wide_source == NULL )
		goto done;
	if( source_size > 0 )
		memcpy( wide_source, source->data, (size_t)source_size );
	wide_source[source_chars] = 0;

    chars_required = WideCharToMultiByte( dest_cp,
                                          0,
                                          wide_source,
                                          source_chars,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL );
	if( (source_chars > 0) && (chars_required <= 0) )
		goto done;
	res = fb_hStrAllocTemp_NoLock( NULL, chars_required );
	if( res == NULL ) {
		res = &fb_ctx.null_desc;
		goto done;
	}
	if( chars_required > 0 ) {
		chars_converted = WideCharToMultiByte( dest_cp, 0, wide_source,
		                                      source_chars, res->data,
		                                      chars_required, NULL, NULL );
		if( chars_converted != chars_required ) {
			fb_hStrDelTemp_NoLock( res );
			res = &fb_ctx.null_desc;
			goto done;
		}
	}
	res->data[chars_required] = 0;

done:
	free( wide_source );
    fb_hStrDelTemp_NoLock( source );
    FB_STRUNLOCK();

    return res;
}

FBSTRING_ACCESS *fb_hIntlConvertString( FBSTRING_ACCESS *source,
                                 int source_cp,
                                 int dest_cp )
{
	if( (source_cp < 0) || (dest_cp < 0) ) {
		fb_hStrDelTemp( source );
		return &fb_ctx.null_desc;
	}
    return fb_hIntlConvertFromWC(
	    fb_hIntlConvertToWC( source, (UINT)source_cp ),
	    (UINT)dest_cp );
}

/* ------------------------------------------------------------------------- */
/* Locale-string retrieval                                                   */
/* ------------------------------------------------------------------------- */

char *fb_hGetLocaleInfo( LCID locale,
                         LCTYPE locale_type,
                         char *buffer,
                         size_t buffer_size )
{
    WCHAR *wide_buffer;
    char *result;
    int narrow_required;
    int wide_required;

    wide_required = GetLocaleInfoW( locale, locale_type, NULL, 0 );
    if( wide_required <= 0 ||
        (size_t)wide_required > (SIZE_MAX / sizeof( WCHAR )) )
        return NULL;

    wide_buffer = (WCHAR *)malloc(
        (size_t)wide_required * sizeof( WCHAR ) );
    if( wide_buffer==NULL )
        return NULL;

    if( GetLocaleInfoW( locale,
                        locale_type,
                        wide_buffer,
                        wide_required )==0 ) {
        free( wide_buffer );
        return NULL;
    }

    narrow_required = WideCharToMultiByte( CP_ACP,
                                           0,
                                           wide_buffer,
                                           wide_required,
                                           NULL,
                                           0,
                                           NULL,
                                           NULL );
    if( narrow_required <= 0 ) {
        free( wide_buffer );
        return NULL;
    }

    if( buffer_size==0 ) {
        result = (char *)malloc( (size_t)narrow_required );
        buffer_size = (size_t)narrow_required;
    } else {
        result = buffer;
    }

    if( result==NULL ||
        buffer_size > (size_t)INT_MAX ||
        (size_t)narrow_required > buffer_size ||
        WideCharToMultiByte( CP_ACP,
                             0,
                             wide_buffer,
                             wide_required,
                             result,
                             (int)buffer_size,
                             NULL,
                             NULL )==0 ) {
        if( buffer_size==(size_t)narrow_required && result!=buffer )
            free( result );
        result = NULL;
    }

    free( wide_buffer );
    return result;
}

/* end of drivers/wince/intl_core.c */
