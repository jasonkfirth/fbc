/*
 * Project: FreeBASIC runtime library
 * File: drivers/win32/intl_core.c
 * Purpose: Core i18n functions.
 * Responsibilities: Target-specific runtime support for win32.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_intl.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/*
    String data can start at byte alignment. Keep UTF-16 conversion buffers
    naturally aligned before passing them to the Windows conversion APIs.
*/
/* Convert a strings character set to another character set. */
static FBSTRING_ACCESS *fb_hIntlConvertToWC( FBSTRING_ACCESS *source, UINT source_cp )
{
    FBSTRING_ACCESS *res;
    ssize_t source_size;
    size_t wide_size, string_size;
    const char *source_data;
    int source_chars, CharsRequired;
    WCHAR *wide_buffer;

    FB_STRLOCK();

    res = &fb_ctx.null_desc;
    wide_buffer = NULL;
    if( source==NULL )
        goto done;

    /* Win32 code-page APIs take signed int lengths. */
    source_size = FB_STRSIZE(source);
    if( (source_size<0) || (source_size>INT_MAX) )
        goto done;
    source_chars = (int)source_size;
    if( (source_chars>0) && (source->data==NULL) )
        goto done;
    source_data = (source_chars>0) ? source->data : "";

    CharsRequired =
        MultiByteToWideChar( source_cp,
                             0,
                             source_data,
                             source_chars,
                             NULL,
                             0 );

    if( CharsRequired<0 )
        goto done;
    /* The runtime string descriptor stores lengths as signed byte counts. */
    if( (size_t)CharsRequired > (((size_t)FB_STRSIZEMSK - 1u) / sizeof(WCHAR)) )
        goto done;

    wide_size = (size_t)CharsRequired * sizeof(WCHAR);
    string_size = wide_size + 1u;
    wide_buffer = (WCHAR *)calloc( 1, wide_size + sizeof(WCHAR) );
    if( wide_buffer==NULL )
        goto done;

    if( CharsRequired>0 ) {
        int chars_converted;

        chars_converted =
            MultiByteToWideChar( source_cp,
                                 0,
                                 source_data,
                                 source_chars,
                                 wide_buffer,
                                 CharsRequired );
        if( chars_converted!=CharsRequired )
            goto done;
    }

    res = fb_hStrAllocTemp_NoLock( NULL, (ssize_t)string_size );
    if( res==NULL ) {
        res = &fb_ctx.null_desc;
    } else {
        memcpy( res->data, wide_buffer, wide_size + sizeof(WCHAR) );
    }

done:
    free( wide_buffer );
    fb_hStrDelTemp_NoLock( source );
    FB_STRUNLOCK();
    return res;
}

static FBSTRING_ACCESS *fb_hIntlConvertFromWC( FBSTRING_ACCESS *source, UINT dest_cp )
{
    FBSTRING_ACCESS *res;
    ssize_t source_size;
    size_t wide_count, wide_size;
    int wide_chars, CharsRequired;
    WCHAR empty_wchar = 0;
    WCHAR *wide_buffer = &empty_wchar;

    FB_STRLOCK();

    res = &fb_ctx.null_desc;
    if( source==NULL )
        goto done;

    source_size = FB_STRSIZE(source);
    if( source_size<0 )
        goto done;
    wide_count = (size_t)source_size / sizeof(WCHAR);
    if( wide_count > (size_t)INT_MAX )
        goto done;
    wide_chars = (int)wide_count;
    wide_size = wide_count * sizeof(WCHAR);

    if( wide_count>0 ) {
        if( source->data==NULL )
            goto done;
        wide_buffer = (WCHAR *)malloc( wide_size );
        if( wide_buffer==NULL )
            goto done;
        memcpy( wide_buffer, source->data, wide_size );
    }

    CharsRequired =
        WideCharToMultiByte( dest_cp,
                             0,
                             wide_buffer,
                             wide_chars,
                             (LPSTR) NULL,
                             0,
                             NULL,
                             NULL );

    if( CharsRequired<0 )
        goto done;
    res = fb_hStrAllocTemp_NoLock( NULL, (ssize_t)CharsRequired );
    if( res==NULL ) {
        res = &fb_ctx.null_desc;
        goto done;
    }

    if( CharsRequired>0 ) {
        int chars_converted;

        chars_converted =
            WideCharToMultiByte( dest_cp,
                                 0,
                                 wide_buffer,
                                 wide_chars,
                                 res->data,
                                 CharsRequired,
                                 NULL,
                                 NULL );
        if( chars_converted!=CharsRequired ) {
            fb_hStrDelTemp_NoLock( res );
            res = &fb_ctx.null_desc;
            goto done;
        }
    }
    res->data[(size_t)CharsRequired] = 0;

done:
    if( wide_buffer != &empty_wchar )
        free( wide_buffer );
    fb_hStrDelTemp_NoLock( source );
    FB_STRUNLOCK();
    return res;
}

FBSTRING_ACCESS *fb_hIntlConvertString( FBSTRING_ACCESS *source, int source_cp, int dest_cp )
{
	return fb_hIntlConvertFromWC( fb_hIntlConvertToWC( source, (UINT)source_cp ),
	                              (UINT)dest_cp );
}

char *fb_hGetLocaleInfo( LCID Locale, LCTYPE LCType, char *pszBuffer, size_t uiSize )
{
    if( uiSize==0 ) {
        /* Keep the Win32 count (uiSize - 1) representable as int. */
        size_t max_buffer_size = (size_t)INT_MAX + 1u;

        uiSize = 64;
        pszBuffer = NULL;
        for(;;) {
            char* pszNewBuffer;

            if( uiSize > (max_buffer_size / 2u) ) {
                free( pszBuffer );
                pszBuffer = NULL;
                break;
            }
            uiSize *= 2u;
            pszNewBuffer = (char*) realloc( pszBuffer, uiSize );
            if( pszNewBuffer==NULL ) {
                free(pszBuffer);
                break;
            }
            pszBuffer = pszNewBuffer;
            if( GetLocaleInfo( Locale, LCType, pszBuffer, (int)(uiSize - 1u) )!=0 ) {
                return pszBuffer;
            }
            if( GetLastError( ) != ERROR_INSUFFICIENT_BUFFER ) {
                free( pszBuffer );
                pszBuffer = NULL;
                break;
            }
        }
    } else {
        if( (pszBuffer==NULL) || (uiSize>(size_t)INT_MAX) )
            return NULL;
        if( GetLocaleInfo( Locale, LCType, pszBuffer, (int)uiSize )!=0 )
            return pszBuffer;
    }
    return NULL;
}

/* end of drivers/win32/intl_core.c */
