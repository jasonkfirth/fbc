/*
    FreeBASIC graphics libraries: gfx_font.h

    Decode DRAW STRING's byte-font row and measure glyph advances. Both
    renderers use this reader so drawing and measurement accept the same
    width tables. Image ownership, image-header parsing and rendering remain
    with the caller. A valid image allocation must cover its declared rows.
*/

#ifndef FB_GFX_FONT_H
#define FB_GFX_FONT_H

#include <limits.h>
#include <stdint.h>
#include <string.h>

typedef struct FB_GFX_FONT {
	uint32_t font_height;
	uint32_t first, last;
	uint32_t glyph_offsets[256];
	uint32_t glyph_widths[256];
} FB_GFX_FONT;

/*
    Row zero contains version 0, first byte, last byte, then one width byte
    per glyph. The remaining rows contain the glyphs side by side. Widths
    describe visible pixels; row padding cannot hold extra glyph pixels.
    Unsupported bytes advance by the full font height, even when clipped.
*/
static int fb_hGfxParseFont(const unsigned char *pixels, uint32_t width,
	uint32_t height, uint32_t pitch, FB_GFX_FONT *font)
{
	uint32_t first, last, code, advance, offset = 0;

	if (!pixels || !font || !width || height <= 1 ||
	    height > INT_MAX || pitch < 4 || pixels[0] != 0)
		return 0;
	first = pixels[1];
	last = pixels[2];
	if (first > last) {
		uint32_t temporary = first;
		first = last;
		last = temporary;
	}
	if (last - first + 4 > pitch)
		return 0;
	memset(font, 0, sizeof(*font));
	font->font_height = height - 1;
	font->first = first;
	font->last = last;
	for (code = first; code <= last; code++) {
		advance = pixels[3 + code - first];
		if (advance > width - offset)
			return 0;
		font->glyph_offsets[code] = offset;
		font->glyph_widths[code] = advance;
		offset += advance;
	}
	return 1;
}

/* Dimensions use the public 32-bit pixel ABI. Never wrap a long caption. */
static int fb_hGfxMeasureFont(const unsigned char *text, size_t length,
	const FB_GFX_FONT *font, int builtin_width, int builtin_height,
	int *width, int *height)
{
	size_t i;
	uint32_t code, advance, total = 0;

	if ((!text && length) || !width || !height)
		return 0;
	if (!font) {
		if (builtin_width <= 0 || builtin_height <= 0 ||
		    length > (size_t)INT_MAX / (unsigned int)builtin_width)
			return 0;
		total = (uint32_t)length * builtin_width;
		*height = builtin_height;
	} else {
		for (i = 0; i < length; i++) {
			code = text[i];
			advance = (code >= font->first && code <= font->last) ?
				font->glyph_widths[code] : font->font_height;
			if (advance > INT_MAX - total)
				return 0;
			total += advance;
		}
		*height = font->font_height;
	}
	*width = total;
	return 1;
}

#endif

/* end of gfx_font.h */
