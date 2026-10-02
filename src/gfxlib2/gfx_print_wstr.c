/*
    FreeBASIC gfxlib2
    File: gfx_print_wstr.c
    Purpose: Print wide text through the byte-indexed bitmap console.
    Responsibilities: Decode whole scalars and release conversion temporaries.
    This file intentionally does NOT contain console state or glyph rendering.
*/

#include "fb_gfx.h"
#include "gfx_unicode.h"

void fb_GfxPrintBufferWstrEx(const FB_WCHAR *buffer, size_t len, int mask)
{
	FBSTRING *glyphs = fb_hGfxWideGlyphs(buffer, len);
	if( glyphs == NULL ) return;
	fb_GfxPrintBufferEx(glyphs->data, FB_STRSIZE(glyphs), mask);
	fb_hStrDelTemp(glyphs);
}

void fb_GfxPrintBufferWstr(const FB_WCHAR *buffer, int mask)
{
	fb_GfxPrintBufferWstrEx( buffer, fb_wstr_Len(buffer), mask);
}

/* end of gfx_print_wstr.c */
