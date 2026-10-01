/*
    FreeBASIC gfxlib2
    File: gfx_drawstring_utf8.c
    Purpose: Route wide and UTF-8 text through the bitmap font renderer.
    Responsibilities: Decode whole scalars and preserve temporary ownership.
    This file intentionally does NOT contain glyph rasterization or fonts.

    Bitmap fonts address glyphs by one-byte values. Unicode scalars through
    U+00FF select that glyph index; larger scalars select the '?' fallback.
    Decoding first keeps a supplementary scalar from becoming four glyphs.
*/

#include "fb_gfx.h"

static FBSTRING *hGlyphs( FBSTRING *text )
{
	FBSTRING *result;
	ssize_t offset = 0, written = 0, length = text && text->data ? FB_STRSIZE(text) : 0;
	ssize_t count = fb_hUtf8Count(text ? text->data : NULL, length);
	unsigned int scalar;
	FB_STRLOCK();
	result = fb_hUStrAlloc_NoLock(count);
	if( result != NULL && count > 0 ) {
		while( offset < length ) {
			scalar = fb_hUtf8Decode(text->data, length, &offset);
			result->data[written++] = scalar <= 0xFF ? scalar : '?';
		}
	}
	fb_hStrDelTemp_NoLock(text);
	FB_STRUNLOCK();
	return result ? result : &__fb_ctx.null_desc;
}

FBCALL int fb_GfxDrawStringUstr( void *target, float x, float y, int flags,
	FBSTRING *text, unsigned int color, void *font, int mode,
	PUTTER *putter, BLENDER *blender, void *parameter )
{
	return fb_GfxDrawString(target, x, y, flags, hGlyphs(text), color,
		font, mode, putter, blender, parameter);
}

FBCALL int fb_GfxDrawStringSizeUstr( FBSTRING *text, int *width, int *height, void *font )
{
	return fb_GfxDrawStringSize(hGlyphs(text), width, height, font);
}

FBCALL int fb_GfxDrawStringSizeWstr( const FB_WCHAR *text, int *width, int *height, void *font )
{
	return fb_GfxDrawStringSizeUstr(fb_UStrFromWstr(text), width, height, font);
}

/* end of gfx_drawstring_utf8.c */
