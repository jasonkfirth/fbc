/*
    FreeBASIC Runtime Library
    -------------------------

    File: strw_convto_lng.c

    Purpose:

        Convert signed and unsigned 64-bit integers to decimal WSTRINGs.

    Responsibilities:

        - allocate temporary wide-string results
        - avoid optional Microsoft CRT conversion helpers on MinGW
        - preserve platform-native wide formatting elsewhere

    This file intentionally does NOT contain:

        - narrow-string conversion
        - QuickBASIC leading-space behavior
        - arbitrary-radix conversion
*/

#include "../fb.h"

/* ------------------------------------------------------------------------- */
/* MinGW decimal conversion                                                  */
/* ------------------------------------------------------------------------- */

#ifdef HOST_MINGW
static int fb_hCopyLongintToWstr( FB_WCHAR *dst, size_t dst_chars,
                                 const char *src, int chars )
{
	size_t i;

	if( (chars < 0) || ((size_t)chars >= dst_chars) )
		return FALSE;

	for( i = 0; i <= (size_t)chars; ++i )
		dst[i] = (FB_WCHAR)(unsigned char)src[i];

	return TRUE;
}

static int fb_hLongintToWstr( FB_WCHAR *dst, size_t dst_chars, long long num )
{
	char buffer[sizeof( long long ) * 3 + 1];
	int chars;

	chars = snprintf( buffer, sizeof( buffer ), "%lld", num );
	return fb_hCopyLongintToWstr( dst, dst_chars, buffer, chars );
}

static int fb_hULongintToWstr( FB_WCHAR *dst, size_t dst_chars,
                              unsigned long long num )
{
	char buffer[sizeof( unsigned long long ) * 3 + 1];
	int chars;

	chars = snprintf( buffer, sizeof( buffer ), "%llu", num );
	return fb_hCopyLongintToWstr( dst, dst_chars, buffer, chars );
}
#endif

/* ------------------------------------------------------------------------- */
/* Public conversion routines                                                */
/* ------------------------------------------------------------------------- */

FBCALL FB_WCHAR *fb_LongintToWstr ( long long num )
{
	FB_WCHAR *dst;

	/* alloc temp string */
    dst = fb_wstr_AllocTemp( sizeof( long long ) * 3 );
	if( dst != NULL )
	{
		/* convert */
#ifdef HOST_MINGW
		if( !fb_hLongintToWstr( dst, sizeof( long long ) * 3 + 1, num ) ) {
			fb_wstr_Del( dst );
			dst = NULL;
		}
#else
        FB_WSTR_FROM_INT64( dst, num );
#endif
	}

	return dst;
}

/*:::::*/
FBCALL FB_WCHAR *fb_ULongintToWstr ( unsigned long long num )
{
	FB_WCHAR *dst;

	/* alloc temp string */
    dst = fb_wstr_AllocTemp( sizeof( long long ) * 3 );
	if( dst != NULL )
	{
        /* convert */
#ifdef HOST_MINGW
		if( !fb_hULongintToWstr( dst, sizeof( unsigned long long ) * 3 + 1,
		                          num ) ) {
			fb_wstr_Del( dst );
			dst = NULL;
		}
#else
        FB_WSTR_FROM_UINT64( dst, num );
#endif
	}

	return dst;
}

/* end of strw_convto_lng.c */
