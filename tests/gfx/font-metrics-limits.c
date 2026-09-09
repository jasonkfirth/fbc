/*
    FreeBASIC graphics tests: font-metrics-limits.c
    Check the shared byte-font parser and dimension limits independently of
    allocation and rendering. Does not substitute for public ABI pixel tests.
*/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../src/gfxlib2/gfx_font.h"

static void require(int condition)
{
	if (!condition)
		abort();
}

int main(void)
{
	FB_GFX_FONT font;
	unsigned char row[5] = { 0, 65, 66, 3, 2 };
	int width, height;

	require(fb_hGfxParseFont(row, 5, 6, sizeof(row), &font));
	require(fb_hGfxMeasureFont((unsigned char *)"AB", 2, &font, 0, 0, &width, &height));
	require(width == 5 && height == 5);
	require(!fb_hGfxMeasureFont(NULL, 1, &font, 0, 0, &width, &height));
	require(fb_hGfxMeasureFont(NULL, 0, &font, 0, 0, &width, &height));
	require(width == 0 && height == 5);
	require(!fb_hGfxMeasureFont(row, (size_t)INT_MAX / 8 + 1, NULL, 8, 16, &width, &height));
	font.font_height = INT_MAX - 1u;
	require(!fb_hGfxMeasureFont((unsigned char *)"XX", 2, &font, 0, 0, &width, &height));
	row[1] = 0; row[2] = 255;
	require(!fb_hGfxParseFont(row, 5, 6, sizeof(row), &font));
	row[1] = 65; row[2] = 66; row[3] = 5;
	require(!fb_hGfxParseFont(row, 5, 6, sizeof(row), &font));
	require(!fb_hGfxParseFont(row, 5, UINT32_MAX, sizeof(row), &font));
	puts("font-metrics-limits: passed");
	return 0;
}

/* end of font-metrics-limits.c */
