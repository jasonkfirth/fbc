/*
    FreeBASIC graphics libraries
    File: gfx_unicode.h
    Purpose: Share wide scalar decoding for the byte-indexed bitmap console.
    Responsibilities: Keep UTF-16 pairs intact and select fallback glyphs.
    This file intentionally does NOT contain rendering or console state.
*/

#ifndef FB_GFX_UNICODE_H
#define FB_GFX_UNICODE_H

/* The caller owns the returned temporary descriptor. Only its allocation
   uses the string lock; drawing takes the graphics lock after conversion. */
static FBSTRING *fb_hGfxWideGlyphs( const FB_WCHAR *buffer, size_t length )
{
	FBSTRING *glyphs;
	size_t i, written = 0;
	unsigned int scalar;
	if( buffer == NULL && length != 0 ) return NULL;
	if( length > (size_t)FB_USTRING_MAX_BYTES ) {
		fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
		return NULL;
	}
	FB_STRLOCK();
	glyphs = fb_hUStrAlloc_NoLock(length);
	if( glyphs != NULL && length > 0 ) {
		for( i = 0; i < length; ++i ) {
			scalar = buffer[i];
			if( sizeof(FB_WCHAR) == 2 && scalar >= 0xD800 && scalar <= 0xDBFF &&
			    i + 1 < length && buffer[i + 1] >= 0xDC00 && buffer[i + 1] <= 0xDFFF ) {
				scalar = 0x10000 + ((scalar - 0xD800) << 10) + buffer[i + 1] - 0xDC00;
				++i;
			}
			glyphs->data[written++] = scalar <= 0xFF ? scalar : '?';
		}
		fb_hStrSetLength(glyphs, written);
		glyphs->data[written] = 0;
	}
	FB_STRUNLOCK();
	return glyphs;
}

#endif

/* end of gfx_unicode.h */
