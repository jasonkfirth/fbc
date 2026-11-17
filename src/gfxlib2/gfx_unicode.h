/*
    FreeBASIC graphics libraries
    File: gfx_unicode.h
    Purpose: Share wide scalar decoding for the byte-indexed bitmap console.
    Responsibilities: Keep UTF-16 pairs intact and select fallback glyphs.
    This file intentionally does NOT contain rendering or console state.
*/

#ifndef FB_GFX_UNICODE_H
#define FB_GFX_UNICODE_H

#include <stdint.h>

/* The caller owns the returned temporary descriptor. Only its allocation
   uses the string lock; drawing takes the graphics lock after conversion. */
static FBSTRING_ACCESS *fb_hGfxWideGlyphs( const FB_WCHAR *buffer, size_t length )
{
	FBSTRING_ACCESS *glyphs;
	size_t i, written = 0;
	unsigned int scalar;
	if( buffer == NULL && length != 0 ) return NULL;
	if( length > (size_t)FB_USTRING_MAX_BYTES ) {
		fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
		return NULL;
	}
	FB_STRLOCK();
	glyphs = fb_hUStrAlloc_NoLock((ssize_t)length);
	if( glyphs != NULL && length > 0 ) {
		for( i = 0; i < length; ++i ) {
			if( sizeof(FB_WCHAR) == 2 ) {
				scalar = (unsigned int)(uint16_t)buffer[i];
				if( scalar >= 0xD800 && scalar <= 0xDBFF &&
				    i + 1 < length && (uint16_t)buffer[i + 1] >= 0xDC00 &&
				    (uint16_t)buffer[i + 1] <= 0xDFFF ) {
					scalar = 0x10000u + ((scalar - 0xD800u) << 10) +
						(unsigned int)(uint16_t)buffer[i + 1] - 0xDC00u;
					++i;
				}
			} else {
				scalar = (unsigned int)buffer[i];
			}
			glyphs->data[written++] = (char)(unsigned char)(scalar <= 0xFF ? scalar : '?');
		}
		fb_hStrSetLength(glyphs, written);
		glyphs->data[written] = 0;
	}
	FB_STRUNLOCK();
	return glyphs;
}

#endif

/* end of gfx_unicode.h */
