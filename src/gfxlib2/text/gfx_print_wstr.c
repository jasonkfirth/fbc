/*
    FreeBASIC gfxlib2
    File: gfx_print_wstr.c
    Purpose: Print wide text through the byte-indexed bitmap console.
    Responsibilities: Decode whole scalars and release conversion temporaries.
    This file intentionally does NOT contain console state or glyph rendering.
*/

#include "../fb_gfx.h"
#include "../gfx_unicode.h"

void fb_GfxPrintBufferWstrEx(const FB_WCHAR *buffer, size_t len, int mask)
{
	FBSTRING_ACCESS *glyphs = fb_hGfxWideGlyphs(buffer, len);
	if( glyphs == NULL ) return;
	fb_GfxPrintBufferEx(glyphs->data, FB_STRSIZE(glyphs), mask);
	fb_hStrDelTemp(glyphs);
}

void fb_GfxPrintBufferWstr(const FB_WCHAR *buffer, int mask)
{
	ssize_t length = fb_wstr_Len(buffer);
	if (length < 0) {
		fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		return;
	}
	fb_GfxPrintBufferWstrEx(buffer, (size_t)length, mask);
}

/* end of gfx_print_wstr.c */
