/*
    FreeBASIC graphics libraries
    File: gfx_pattern_utf8.c
    Purpose: Supply Unicode arguments to the packed-byte paint pattern API.
    Responsibilities: Encode wide patterns as UTF-8 and preserve ownership.
    This file intentionally does NOT interpret glyphs or implement flood fill.
*/

#include "fb_gfx.h"

FBCALL int fb_GfxPaintPatternWstr( void *target, float x, float y,
	const FB_WCHAR *pattern, unsigned int foreground, unsigned int background,
	unsigned int border, int relative )
{
	FBSTRING *bytes;
	/* A pattern is binary: one encoded byte per row. UTF-8 conversion gives
	   wide and USTRING callers the same bytes without a locale dependency. */
	FB_STRLOCK();
	bytes = fb_hUStrFromWstr_NoLock(pattern);
	FB_STRUNLOCK();
	if( bytes == NULL ) return FB_RTERROR_OUTOFMEM;
	return fb_GfxPaintPattern(target, x, y, bytes, foreground, background, border, relative);
}

/* end of gfx_pattern_utf8.c */
