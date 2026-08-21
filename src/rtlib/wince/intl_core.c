/*
    FreeBASIC Runtime Library support for Windows CE
    ------------------------------------------------

    File: intl_core.c

    Purpose:

        Implement character-set conversion and locale-string retrieval for
        the Unicode-only Windows CE API surface.

    Responsibilities:

        - convert temporary FreeBASIC strings through UTF-16
        - query locale data with GetLocaleInfoW
        - expose locale values as narrow strings expected by the shared API
        - support caller-owned and runtime-allocated result buffers

    This file intentionally does NOT contain:

        - date or time format parsing
        - locale selection policy
        - desktop Windows ANSI API assumptions
*/

#include "../fb.h"
#include "fb_private_intl.h"

#include <limits.h>

/* ------------------------------------------------------------------------- */
/* Character-set conversion                                                  */
/* ------------------------------------------------------------------------- */

static FBSTRING *fb_hIntlConvertToWC( FBSTRING *source, UINT source_cp )
{
    FBSTRING *res;
    int chars_required;

    FB_STRLOCK();

    chars_required = MultiByteToWideChar( source_cp,
                                          0,
                                          source->data,
                                          FB_STRSIZE( source ),
                                          NULL,
                                          0 );

    res = fb_hStrAllocTemp_NoLock(
        NULL,
        (chars_required + 1) * sizeof( WCHAR ) - 1 );

    if( res!=NULL ) {
        size_t terminator_index = chars_required * sizeof( WCHAR );

        MultiByteToWideChar( source_cp,
                             0,
                             (LPCSTR)source->data,
                             FB_STRSIZE( source ),
                             (LPWSTR)res->data,
                             chars_required );
        *((WCHAR *)(res->data + terminator_index)) = 0;
    } else {
        res = &__fb_ctx.null_desc;
    }

    fb_hStrDelTemp_NoLock( source );
    FB_STRUNLOCK();

    return res;
}

static FBSTRING *fb_hIntlConvertFromWC( FBSTRING *source, UINT dest_cp )
{
    FBSTRING *res;
    int chars_required;

    FB_STRLOCK();

    chars_required = WideCharToMultiByte( dest_cp,
                                          0,
                                          (LPCWSTR)source->data,
                                          FB_STRSIZE( source ) / sizeof( WCHAR ),
                                          NULL,
                                          0,
                                          NULL,
                                          NULL );

    res = fb_hStrAllocTemp_NoLock( NULL, chars_required );
    if( res!=NULL ) {
        WideCharToMultiByte( dest_cp,
                             0,
                             (LPCWSTR)source->data,
                             FB_STRSIZE( source ) / sizeof( WCHAR ),
                             (LPSTR)res->data,
                             chars_required,
                             NULL,
                             NULL );
        res->data[chars_required] = 0;
    } else {
        res = &__fb_ctx.null_desc;
    }

    fb_hStrDelTemp_NoLock( source );
    FB_STRUNLOCK();

    return res;
}

FBSTRING *fb_hIntlConvertString( FBSTRING *source,
                                 int source_cp,
                                 int dest_cp )
{
    return fb_hIntlConvertFromWC(
        fb_hIntlConvertToWC( source, source_cp ),
        dest_cp );
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

/* end of intl_core.c */
