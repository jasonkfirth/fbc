/*
    FreeBASIC gfxlib2: gfx_drawstring.c

    Draw and measure byte strings using built-in or image-based fonts.
    This file owns font-image validation, glyph spacing and clipped drawing.
    It does not implement console layout, scalable fonts or GUI text units.
*/

#include "fb_gfx.h"
#include "gfx_font.h"

/*
 *	User font format:
 *
 *	Basically a GET/PUT buffer, where the first pixels data line holds the
 *	font header:
 *
 *	offset	|	description
 *	--------+--------------------------------------------------------------
 *	0		|	Font header version (currently must be 0)
 *	1		|	First ascii character supported
 *	2		|	Last ascii character supported
 *	3-(3+n)	|	n-th supported character width
 *
 *	The font height is computed as the height of the buffer minus 1, and the
 *	actual glyph shapes start on the second buffer line, one after another in
 *	the same row, starting with first supported ascii character up to the
 *	last one.
 *
 */

static int parse_font(void *image, int target_bpp, FB_GFX_FONT *font,
	unsigned char **pixels, int *pitch)
{
	PUT_HEADER header;
	uint32_t first_word, width, height, bpp, row_pitch;
	size_t header_size;

	if (!image)
		return 0;
	memcpy(&first_word, image, sizeof(first_word));
	if (first_word == PUT_HEADER_NEW) {
		memcpy(&header, image, sizeof(header));
		bpp = header.bpp;
		width = header.width;
		height = header.height;
		row_pitch = header.pitch;
		header_size = sizeof(header);
	} else {
		bpp = first_word & 7;
		if (!bpp)
			bpp = target_bpp;
		width = (first_word >> 3) & 0x1FFF;
		height = first_word >> 16;
		row_pitch = width * bpp;
		header_size = 4;
	}
	if ((bpp != 1 && bpp != 2 && bpp != 4) ||
	    (target_bpp && bpp != (unsigned int)target_bpp) ||
	    !width || width > INT_MAX / bpp || height > INT_MAX ||
	    row_pitch > INT_MAX || row_pitch < width * bpp ||
	    (height && row_pitch > (SIZE_MAX - header_size) / height))
		return 0;
	*pixels = (unsigned char *)image + header_size;
	*pitch = row_pitch;
	return fb_hGfxParseFont(*pixels, width, height, row_pitch, font);
}

FBCALL int fb_GfxDrawStringSize(FBSTRING *string, int *width, int *height,
	void *font_image)
{
	FB_GFX_FONT font;
	unsigned char *pixels;
	int pitch, measured_width, measured_height;
	int result = FB_RTERROR_ILLEGALFUNCTIONCALL;

	if (width) *width = 0;
	if (height) *height = 0;
	FB_GRAPHICS_LOCK();
	if (!width || !height || width == height || !string ||
	    (!string->data && FB_STRSIZE(string)))
		goto done;
	/* Explicit fonts do not require an active display or matching screen depth. */
	if (font_image) {
		if (!parse_font(font_image, 0, &font, &pixels, &pitch))
			goto done;
	} else if (!__fb_gfx) {
		goto done;
	}
	if (fb_hGfxMeasureFont((unsigned char *)string->data, FB_STRSIZE(string),
	    font_image ? &font : NULL, __fb_gfx ? __fb_gfx->font->w : 0,
	    __fb_gfx ? __fb_gfx->font->h : 0, &measured_width, &measured_height)) {
		*width = measured_width;
		*height = measured_height;
		result = FB_RTERROR_OK;
	}
done:
	fb_hStrDelTemp(string);
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(result);
}

FBCALL int fb_GfxDrawString
	(
		void *target,
		float fx,
		float fy,
		int flags,
		FBSTRING *string,
		unsigned int color,
		void *font,
		int mode,
		PUTTER *putter,
		BLENDER *blender,
		void *param
	)
{
	FB_GFXCTX *context;
	FB_GFX_FONT custom_font;
	int font_height, full_font_height, x, y, px, py, i, w, h, pitch, bpp, first, last;
	int offset, bytes_count, res = fb_ErrorSetNum( FB_RTERROR_OK );
	unsigned char *data, *glyph_pixels;
	unsigned int code, advance;

	FB_GRAPHICS_LOCK( );

	context = fb_hGetContext();

	if ((!__fb_gfx) || (!string) || (!string->data)) {
		if (!string)
			res = FB_RTERROR_ILLEGALFUNCTIONCALL;
		goto exit_error_unlocked;
	}

	fb_hPrepareTarget(context, target);

	if (mode != PUT_MODE_ALPHA) {
		if (flags & DEFAULT_COLOR_1)
			color = context->fg_color;
		else
			color = fb_hFixColor(context->target_bpp, color);
	}

	fb_hSetPixelTransfer(context, color);

	fb_hFixRelative(context, flags, &fx, &fy, NULL, NULL);

	fb_hTranslateCoord(context, fx, fy, &x, &y);

	DRIVER_LOCK();

	if (font) {
		/* user passed a custom font */

		bpp = context->target_bpp;
		if (!parse_font(font, bpp, &custom_font, &data, &pitch)) {
			res = FB_RTERROR_ILLEGALFUNCTIONCALL;
			goto exit_error;
		}
		font_height = full_font_height = custom_font.font_height;
		if (((int64_t)y + font_height <= context->view_y) ||
		    (y >= context->view_y + context->view_h))
			goto exit_error;
		data += pitch;
		if (y < context->view_y) {
			data += (size_t)pitch * (context->view_y - y);
			font_height -= (context->view_y - y);
			y = context->view_y;
		}
		if ((int64_t)y + font_height > context->view_y + context->view_h)
			font_height = context->view_y + context->view_h - y;
		glyph_pixels = data;

		for (i = 0; i < (int)FB_STRSIZE(string); i++) {

			if (x >= context->view_x + context->view_w)
				break;

			code = (unsigned char)string->data[i];
			if (code < custom_font.first || code > custom_font.last) {
				/* character not found */
				if (x > INT_MAX - full_font_height)
					break;
				x += full_font_height;
				continue;
			}
			advance = custom_font.glyph_widths[code];
			data = glyph_pixels + (size_t)custom_font.glyph_offsets[code] * bpp;
			w = advance;
			h = font_height;
			px = x;

			if (w && (int64_t)x + w > context->view_x) {

				if (x < context->view_x) {
					data += ((context->view_x - x) * bpp);
					w -= (context->view_x - x);
					px = context->view_x;
				}
				if ((int64_t)px + w > context->view_x + context->view_w)
					w = context->view_x + context->view_w - px;
				putter(data, context->line[y] + (px * bpp), w, h, pitch, context->target_pitch, color, blender, param);

			}
			if (x > INT_MAX - (int)advance)
				break;
			x += advance;
		}
	} else {
		/* use default font */

		font_height = __fb_gfx->font->h;
		w = __fb_gfx->font->w;
		bytes_count = BYTES_PER_PIXEL(w);
		offset = 0;

		if ((x + (w * (int)FB_STRSIZE(string)) <= context->view_x) || (x >= context->view_x + context->view_w) ||
		    (y + font_height <= context->view_y) || (y >= context->view_y + context->view_h)) {
			goto exit_error;
		}

		if (y < context->view_y) {
			offset = (bytes_count * (context->view_y - y));
			font_height -= (context->view_y - y);
			y = context->view_y;
		}
		if (y + font_height > context->view_y + context->view_h)
			font_height -= ((y + font_height) - (context->view_y + context->view_h));

		first = 0;
		if (x < context->view_x) {
			first = (context->view_x - x) / w;
			x += (first * w);
		}
		last = FB_STRSIZE(string);
		if (x + ((last - first) * w) > context->view_x + context->view_w)
			last -= ((x + ((last - first) * w) - (context->view_x + context->view_w)) / w);

		for (i = first; i < last; i++, x += w) {

			if (x + w <= context->view_x)
				continue;

			if (x >= context->view_x + context->view_w)
				break;

			data = (unsigned char *)__fb_gfx->font->data + ((unsigned char)string->data[i] * bytes_count * __fb_gfx->font->h) + offset;
			for (py = 0; py < font_height; py++) {
				for (px = 0; px < w; px++) {
					if ((*data & (1 << (px & 0x7))) && (x + px >= context->view_x) && (x + px < context->view_x + context->view_w))
						context->put_pixel(context, x + px, y + py, color);
					if ((px & 0x7) == 0x7)
						data++;
				}
			}
		}
	}

	SET_DIRTY(context, y, font_height);

exit_error:
	DRIVER_UNLOCK();

exit_error_unlocked:
	fb_hStrDelTemp(string);

	FB_GRAPHICS_UNLOCK( );

	if (res != FB_RTERROR_OK)
		return fb_ErrorSetNum(res);
	else
		return res;
}

/* end of gfx_drawstring.c */
