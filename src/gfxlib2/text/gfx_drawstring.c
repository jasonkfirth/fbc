/*
 * FreeBASIC gfxlib2: gfx_drawstring.c
 * Purpose: Draw bitmap text into a screen or image target.
 * Responsibilities: text layout, character conversion and console hooks.
 * This file intentionally does NOT contain native window creation or event dispatch.
 */

/*
    FreeBASIC gfxlib2: gfx_drawstring.c

    Draw and measure byte strings using built-in or image-based fonts.
    This file owns font-image validation, glyph spacing and clipped drawing.
    It does not implement console layout, scalable fonts or GUI text units.
*/

#include "../fb_gfx.h"
#include "../gfx_font.h"

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

	if (!image || (target_bpp < 0))
		return 0;
	memcpy(&first_word, image, sizeof(first_word));
	if (first_word == PUT_HEADER_NEW) {
		memcpy(&header, image, sizeof(header));
		bpp = (uint32_t)header.bpp;
		width = header.width;
		height = header.height;
		row_pitch = header.pitch;
		header_size = sizeof(header);
	} else {
		uint16_t words[2];

		/* The legacy header contains two native 16-bit words. Treating
		   them as one integer reverses their roles on big-endian targets. */
		memcpy(words, image, sizeof(words));
		bpp = (uint32_t)(words[0] & 7u);
		if (!bpp)
			bpp = (uint32_t)target_bpp;
		width = (uint32_t)(words[0] >> 3);
		height = (uint32_t)words[1];
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
	*pitch = (int)row_pitch;
	return fb_hGfxParseFont(*pixels, width, height, row_pitch, font);
}

FBCALL int fb_GfxDrawStringSize(FBSTRING_ACCESS *string, int *width, int *height,
	void *font_image)
{
	FB_GFX_FONT font;
	unsigned char *pixels;
	ssize_t string_size;
	int pitch, measured_width, measured_height;
	int result = FB_RTERROR_ILLEGALFUNCTIONCALL;

	if (width) *width = 0;
	if (height) *height = 0;
	FB_GRAPHICS_LOCK();
	if (!width || !height || width == height || !string ||
	    (!string->data && FB_STRSIZE(string)))
		goto done;
	string_size = FB_STRSIZE(string);
	if (string_size < 0)
		goto done;
	/* Explicit fonts do not require an active display or matching screen depth. */
	if (font_image) {
		if (!parse_font(font_image, 0, &font, &pixels, &pitch))
			goto done;
	} else if (!fb_gfx) {
		goto done;
	}
	if (fb_hGfxMeasureFont((unsigned char *)string->data, (size_t)string_size,
	    font_image ? &font : NULL, fb_gfx ? fb_gfx->font->w : 0,
	    fb_gfx ? fb_gfx->font->h : 0, &measured_width, &measured_height)) {
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
		FBSTRING_ACCESS *string,
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
	int bytes_count, res = fb_ErrorSetNum( FB_RTERROR_OK );
	size_t offset;
	unsigned char *data, *glyph_pixels;
	const unsigned char *font_data;
	unsigned int code, advance;
	ssize_t string_size;
	int string_length;
	int64_t clipped_amount, text_right, view_right, view_bottom;

	FB_GRAPHICS_LOCK( );

	context = fb_hGetContext();

	if ((!fb_gfx) || (!string) || (!string->data)) {
		if (!string)
			res = FB_RTERROR_ILLEGALFUNCTIONCALL;
		goto exit_error_unlocked;
	}
	string_size = FB_STRSIZE(string);
	if ((string_size < 0) || (string_size > INT_MAX)) {
		res = FB_RTERROR_ILLEGALFUNCTIONCALL;
		goto exit_error_unlocked;
	}
	/* Drawing indices and pixel coordinates use int throughout this path. */
	string_length = (int)string_size;

	fb_hPrepareTarget(context, target);

	if (mode != PUT_MODE_ALPHA) {
		if ((unsigned int)flags & DEFAULT_COLOR_1)
			color = context->fg_color;
		else
			color = fb_hFixColor(context->target_bpp, color);
	}

	fb_hSetPixelTransfer(context, color);

	fb_hFixRelative(context, flags, &fx, &fy, NULL, NULL);

	fb_hTranslateCoord(context, fx, fy, &x, &y);

	DRIVER_LOCK();
	view_right = (int64_t)context->view_x + context->view_w;
	view_bottom = (int64_t)context->view_y + context->view_h;
	if ((context->view_w <= 0) || (context->view_h <= 0) ||
	    (view_right > INT_MAX) || (view_bottom > INT_MAX))
		goto exit_error;

	if (font) {
		/* user passed a custom font */

		bpp = context->target_bpp;
		if (bpp <= 0) {
			res = FB_RTERROR_ILLEGALFUNCTIONCALL;
			goto exit_error;
		}
		if (!parse_font(font, bpp, &custom_font, &data, &pitch)) {
			res = FB_RTERROR_ILLEGALFUNCTIONCALL;
			goto exit_error;
		}
		font_height = full_font_height = (int)custom_font.font_height;
		if (((int64_t)y + font_height <= context->view_y) ||
		    ((int64_t)y >= view_bottom))
			goto exit_error;
		data += pitch;
		if (y < context->view_y) {
			clipped_amount = (int64_t)context->view_y - y;
			if ((clipped_amount <= 0) || (clipped_amount >= font_height) ||
			    ((size_t)clipped_amount > SIZE_MAX / (size_t)pitch))
				goto exit_error;
			data += (size_t)pitch * (size_t)clipped_amount;
			font_height -= (int)clipped_amount;
			y = context->view_y;
		}
		if ((int64_t)y + font_height > view_bottom) {
			clipped_amount = view_bottom - y;
			if (clipped_amount <= 0)
				goto exit_error;
			font_height = (int)clipped_amount;
		}
		glyph_pixels = data;

		for (i = 0; i < string_length; i++) {

			if ((int64_t)x >= view_right)
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
			if (advance > (uint32_t)INT_MAX)
				goto exit_error;
			data = glyph_pixels + (size_t)custom_font.glyph_offsets[code] * (size_t)bpp;
			w = (int)advance;
			h = font_height;
			px = x;

			if (w && (int64_t)x + w > context->view_x) {

				if (x < context->view_x) {
					clipped_amount = (int64_t)context->view_x - x;
					if ((clipped_amount <= 0) || (clipped_amount >= w) ||
					    ((size_t)clipped_amount > SIZE_MAX / (size_t)bpp))
						goto exit_error;
					data += (size_t)clipped_amount * (size_t)bpp;
					w -= (int)clipped_amount;
					px = context->view_x;
				}
				if ((int64_t)px + w > view_right) {
					clipped_amount = view_right - px;
					if (clipped_amount <= 0)
						goto exit_error;
					w = (int)clipped_amount;
				}
				putter(data, context->line[y] + (px * bpp), w, h, pitch, context->target_pitch, (int)color, blender, param);

			}
			if (x > INT_MAX - (int)advance)
				break;
			x += (int)advance;
		}
	} else {
		/* use default font */

		if (!fb_gfx->font || !fb_gfx->font->data)
			goto exit_error;
		font_height = fb_gfx->font->h;
		w = fb_gfx->font->w;
		if ((font_height <= 0) || (w <= 0))
			goto exit_error;
		bytes_count = (w / 8) + ((w % 8) != 0);
		offset = 0;

		if (((int64_t)x + ((int64_t)w * string_length) <= context->view_x) ||
		    ((int64_t)x >= view_right) ||
		    ((int64_t)y + font_height <= context->view_y) ||
		    ((int64_t)y >= view_bottom)) {
			goto exit_error;
		}

		if (y < context->view_y) {
			clipped_amount = (int64_t)context->view_y - y;
			if ((clipped_amount <= 0) || (clipped_amount >= font_height) ||
			    ((size_t)clipped_amount > SIZE_MAX / (size_t)bytes_count))
				goto exit_error;
			offset = (size_t)bytes_count * (size_t)clipped_amount;
			font_height -= (int)clipped_amount;
			y = context->view_y;
		}
		if ((int64_t)y + font_height > view_bottom) {
			clipped_amount = view_bottom - y;
			if (clipped_amount <= 0)
				goto exit_error;
			font_height = (int)clipped_amount;
		}

		first = 0;
		if (x < context->view_x) {
			clipped_amount = ((int64_t)context->view_x - x) / w;
			first = clipped_amount > string_length ? string_length : (int)clipped_amount;
			if (first < string_length)
				x = (int)((int64_t)x + ((int64_t)first * w));
		}
		last = string_length;
		text_right = (int64_t)x + ((int64_t)(last - first) * w);
		if (text_right > view_right) {
			clipped_amount = (text_right - view_right) / w;
			if (clipped_amount > (int64_t)(last - first))
				clipped_amount = last - first;
			last -= (int)clipped_amount;
		}

		for (i = first; i < last; i++, x += w) {

			if ((int64_t)x + w <= context->view_x)
				continue;

			if ((int64_t)x >= view_right)
				break;

			font_data = fb_gfx->font->data + ((size_t)(unsigned char)string->data[i] * (size_t)bytes_count * (size_t)fb_gfx->font->h) + offset;
			for (py = 0; py < font_height; py++) {
				for (px = 0; px < w; px++) {
					if ((*font_data & (1 << (px & 0x7))) && ((int64_t)x + px >= context->view_x) && ((int64_t)x + px < view_right))
						context->put_pixel(context, x + px, y + py, color);
					if ((px & 0x7) == 0x7)
						font_data++;
				}
			}
		}
	}

	if (font_height > 0)
		SET_DIRTY(context, y, (size_t)font_height);

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
