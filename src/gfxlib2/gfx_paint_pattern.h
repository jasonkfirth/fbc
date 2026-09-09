/*
    FreeBASIC graphics libraries: gfx_paint_pattern.h

    Private transport for PaintPattern's two-color, packed byte rows. This
    is shared by the CPU fill implementations; it is not a file format or
    a public ABI and does not contain flood discovery or target selection.
*/

#ifndef FB_GFX_PAINT_PATTERN_H
#define FB_GFX_PAINT_PATTERN_H

#include <stdint.h>
#include <string.h>

#define FB_GFX_PAINT_PACKED 2
#define FB_GFX_PAINT_PACKED_HEADER 8
#define FB_GFX_PAINT_PACKED_MAX_ROWS 64

/* Native uint32 colors occupy bytes 0..7; subsequent bytes are packed rows. */
static int fb_hGfxPackPattern(unsigned char *data, FBSTRING *pattern,
	unsigned int foreground, unsigned int background)
{
	ssize_t rows;
	uint32_t colors[2] = { background, foreground };

	if (!pattern || !pattern->data)
		return 0;
	rows = FB_STRSIZE(pattern);
	if (rows < 1 || rows > FB_GFX_PAINT_PACKED_MAX_ROWS)
		return 0;
	memcpy(data, colors, sizeof(colors));
	memcpy(data + FB_GFX_PAINT_PACKED_HEADER, pattern->data, rows);
	return FB_GFX_PAINT_PACKED_HEADER + (int)rows;
}

static uint32_t fb_hGfxPackedPatternColor(const unsigned char *data,
	size_t size, unsigned int x, unsigned int y)
{
	unsigned int bit;
	uint32_t color;

	/* Callers validate the row count before starting the flood. Bit 7 is left. */
	bit = (data[FB_GFX_PAINT_PACKED_HEADER +
		(y % (size - FB_GFX_PAINT_PACKED_HEADER))] >> (7 - (x & 7))) & 1;
	memcpy(&color, data + bit * sizeof(color), sizeof(color));
	return color;
}

#endif

/* end of gfx_paint_pattern.h */
