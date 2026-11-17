/*
    FreeBASIC Graphics Library
    --------------------------

    File: gfx_bsave.c

    Purpose:

        Implement BSAVE file selection and the legacy BMP/raw writers.

    Responsibilities:

        - select BMP, PNG, or FreeBASIC raw-block output
        - convert gfxlib2 images to BMP storage
        - preserve the public BSAVE runtime entry points

    This file intentionally does NOT contain:

        - BLOAD handling
        - PNG codec internals
        - graphics driver lifecycle code
*/

#include "../fb_gfx.h"
#include "../gfx_png.h"
#include <limits.h>
#include <string.h>
#include <strings.h>
#ifdef HOST_WIN32
	#include <windows.h>
#endif

static int write_byte(FILE *f, unsigned int value)
{
	return (fputc((int)(value & 0xFF), f) != EOF);
}

static int write_le16(FILE *f, unsigned int value)
{
	return write_byte(f, value) &&
	       write_byte(f, value >> 8);
}

static int write_le32(FILE *f, unsigned int value)
{
	return write_byte(f, value) &&
	       write_byte(f, value >> 8) &&
	       write_byte(f, value >> 16) &&
	       write_byte(f, value >> 24);
}

static int write_bmp_header(FILE *f, unsigned int bfSize,
	unsigned int bfOffBits, unsigned int width, unsigned int height,
	unsigned int bitsperpixel, unsigned int biSizeImage,
	unsigned int biClrUsed)
{
	/*
		BMP integers are always little-endian.  Writing the old packed C
		structure directly produced an "MB" signature and reversed every
		header field on big-endian targets such as s390x.
	*/
	return write_le16(f, 0x4D42) && /* 'B' 'M' */
	       write_le32(f, bfSize) &&
	       write_le16(f, 0) &&
	       write_le16(f, 0) &&
	       write_le32(f, bfOffBits) &&
	       write_le32(f, 40) &&
	       write_le32(f, width) &&
	       write_le32(f, height) &&
	       write_le16(f, 1) &&
	       write_le16(f, bitsperpixel) &&
	       write_le32(f, 0) &&
	       write_le32(f, biSizeImage) &&
	       write_le32(f, 0xB12) &&
	       write_le32(f, 0xB12) &&
	       write_le32(f, biClrUsed) &&
	       write_le32(f, biClrUsed);
}

static int save_bmp(FB_GFXCTX *ctx, FILE *f, void *src, void *pal, int outbpp)
{
	PUT_HEADER *put_header;
	int w, h, i, row, inbpp, inpitch;
	unsigned int bfSize, bfOffBits, biSizeImage, biClrUsed, color;
	unsigned char *s, *source, *buffer, *p;
	unsigned short source_pixel;
	unsigned int *palette = (unsigned int *)pal, *paltmp = 0;
	/* Use the maximum size_t value for the overflow checks below. */
	size_t size_max = (size_t)-1;
	size_t row_size, outpitch, image_size, copy_size;

	if (src) {
		put_header = (PUT_HEADER *)src;
		if (put_header->type == PUT_HEADER_NEW) {
			if ((put_header->width > (unsigned int)INT_MAX) ||
			    (put_header->height > (unsigned int)INT_MAX) ||
			    (put_header->pitch > (unsigned int)INT_MAX))
				return FB_RTERROR_ILLEGALFUNCTIONCALL;
			w = (int)put_header->width;
			h = (int)put_header->height;
			s = (unsigned char *)src + sizeof(PUT_HEADER);
			inbpp = put_header->bpp;
			inpitch = (int)put_header->pitch;
		}
		else {
			w = put_header->old.width;
			h = put_header->old.height;
			s = (unsigned char *)src + 4;
			inbpp = (put_header->old.bpp ? put_header->old.bpp : fb_gfx->bpp);
			if ((inbpp <= 0) || (w > (INT_MAX / inbpp)))
				return FB_RTERROR_ILLEGALFUNCTIONCALL;
			inpitch = w * inbpp;
		}
	}
	else {
		w = fb_gfx->w;
		h = fb_gfx->h;
		s = ctx->line[0];
		inbpp = fb_gfx->bpp;
		inpitch = fb_gfx->pitch;
	}

	if ((w <= 0) || (h <= 0) || (s == NULL) || (inbpp <= 0)) {
		/* Something wrong with the image header */
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	}
	if ((w > (INT_MAX / inbpp)) || (inpitch < w * inbpp) ||
	    ((size_t)(h - 1) > (size_max / (size_t)inpitch)))
		return FB_RTERROR_ILLEGALFUNCTIONCALL;

	switch (inbpp) {
	case 1: /* 8-bit or 24-bit output (default to 8) */
		if (outbpp > 8) {
			outbpp = 24;
		}
		else {
			outbpp = 8;
		}
		break;
	case 2: /* 24-bit output only */
		outbpp = 24;
		break;
	case 4: /* 24-bit or 32-bit output only (default to 32) */
		if (outbpp != 24) {
			outbpp = 32;
		}
		break;
	default:
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	}

	/* Change bits/pixel to bytes/pixel */
	outbpp >>= 3;

	/* BMP pixel rows are padded to four-byte boundaries. */
	switch (outbpp) {
	case 1:
		row_size = (size_t)w;
		if (row_size > (size_max - 3u))
			return FB_RTERROR_ILLEGALFUNCTIONCALL;
		outpitch = (row_size + 3u) & ~(size_t)3u;
		/* 54-byte BMP headers plus 256 four-byte palette entries. */
		bfOffBits = 54u + (256u * 4u);
		biClrUsed = 256u;
		break;
	case 3:
		if ((size_t)w > ((size_max - 3u) / 3u))
			return FB_RTERROR_ILLEGALFUNCTIONCALL;
		row_size = (size_t)w * 3u;
		outpitch = (row_size + 3u) & ~(size_t)3u;
		bfOffBits = 54u;
		biClrUsed = 0u;
		break;
	case 4:
	default:
		DBG_ASSERT(outbpp == 4);
		if ((size_t)w > (size_max / 4u))
			return FB_RTERROR_ILLEGALFUNCTIONCALL;
		row_size = (size_t)w * 4u;
		outpitch = row_size;
		bfOffBits = 54u;
		biClrUsed = 0u;
		break;
	}
	if ((outpitch == 0u) || ((size_t)h > (size_max / outpitch)))
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	image_size = outpitch * (size_t)h;
	/* BMP file size fields are 32-bit unsigned values. */
	if (image_size > (size_t)(UINT_MAX - bfOffBits))
		return FB_RTERROR_ILLEGALFUNCTIONCALL;
	bfSize = (unsigned int)(image_size + bfOffBits);
	biSizeImage = (unsigned int)image_size;

	if (!write_bmp_header(f, (unsigned int)bfSize,
	    (unsigned int)bfOffBits, (unsigned int)w, (unsigned int)h,
	    (unsigned int)(outbpp * 8), (unsigned int)biSizeImage,
	    (unsigned int)biClrUsed))
		return FB_RTERROR_FILEIO;

	if (inbpp == 1) {
		if (!pal) {
			palette = fb_gfx->device_palette;
		}
		else {
			paltmp = (unsigned int *)calloc(256, sizeof(unsigned int));
			if (paltmp == 0) {
				return FB_RTERROR_OUTOFMEM;
			}
			for (i = 0; i < 256; i++) {
				paltmp[i] = (palette[i] & 0x3f0000) >> (16 - 2)
				          | (palette[i] & 0x003f00) << 2
				          | (palette[i] & 0x00003f) << (16 + 2);
			}
			palette = paltmp;
		}

		if (outbpp == 1) {
			for (i = 0; i < 256; i++) {
				fputc(((palette[i] >> 16) & 0xFF), f);
				fputc(((palette[i] >> 8) & 0xFF), f);
				fputc((palette[i] & 0xFF), f);
				fputc(0, f);
			}
		}
	}

	buffer = (unsigned char *)calloc(1, outpitch);
	if (buffer == 0) {
		if (paltmp) free(paltmp);
		return FB_RTERROR_OUTOFMEM;
	}

	source = s;
	for (row = h; row > 0; row--) {
		s = source + ((size_t)(row - 1) * (size_t)inpitch);
		p = buffer;
		/* PUT payload bytes are not guaranteed to have native pixel alignment. */
		switch (inbpp) {
			case 1:
				if (outbpp == 1) {
					copy_size = (size_t)inpitch;
					/* Only the padded BMP row is written if source pitch is wider. */
					if (copy_size > outpitch)
						copy_size = outpitch;
					fb_hMemCpy(p, s, copy_size);
				}
				else {
					DBG_ASSERT(outbpp == 3);
					for (i = 0; i < w; i++) {
						color = palette[s[i]];
						*p++ = (unsigned char)((color & 0xFF0000u) >> 16);
						*p++ = (unsigned char)((color & 0x0000FF00u) >> 8);
						*p++ = (unsigned char)(color & 0x000000FFu);
					}
				}
				break;
			case 2:
				DBG_ASSERT(outbpp == 3);
				for (i = 0; i < w; i++) {
					memcpy(&source_pixel,
					       s + ((size_t)i * sizeof(source_pixel)),
					       sizeof(source_pixel));
					color = source_pixel;
					*p++ = (unsigned char)(((color & 0x001Fu) << 3) |
					                       ((color & 0x001Fu) >> 2));
					*p++ = (unsigned char)(((color & 0x07E0u) >> 3) |
					                       ((color & 0x07E0u) >> 9));
					*p++ = (unsigned char)(((color & 0xF800u) >> 8) |
					                       ((color & 0xF800u) >> 13));
				}
				break;
			case 4:
			default:
				DBG_ASSERT(inbpp == 4);
				DBG_ASSERT(outbpp == 3 || outbpp == 4);
				for (i = 0; i < w; i++) {
					memcpy(&color, s + ((size_t)i * sizeof(color)), sizeof(color));
					*p++ = (unsigned char)(color & 0xFFu);
					*p++ = (unsigned char)((color >> 8) & 0xFFu);
					*p++ = (unsigned char)((color >> 16) & 0xFFu);
					if (outbpp == 4)
						*p++ = (unsigned char)((color >> 24) & 0xFFu);
				}
				break;
		}
		if (!fwrite(buffer, outpitch, 1, f)) {
			free(buffer);
			if (paltmp) free(paltmp);
			return FB_RTERROR_FILEIO;
		}
	}

	free(buffer);
	if (paltmp) free(paltmp);

	return fb_ErrorSetNum( FB_RTERROR_OK );
}

FBCALL int fb_GfxBsaveEx(FBSTRING_ACCESS *filename, void *src, unsigned int size, void *pal, int bitsperpixel)
{
	FILE *f;
	FB_GFXCTX *context;
	int i, result = fb_ErrorSetNum( FB_RTERROR_OK );
	unsigned int color, *palette = (unsigned int *)pal;
	char buffer[MAX_PATH], *p;

	FB_GRAPHICS_LOCK( );

	context = fb_hGetContext();
	snprintf(buffer, MAX_PATH-1, "%s", filename->data);
	buffer[MAX_PATH-1] = '\0';
	fb_hConvertPath(buffer);

	/*
		Use the rtlib path layer instead of raw fopen().  Targets such as
		Xbox redirect relative writes to writable storage instead of the
		read-only disc mount.
	*/
	f = fb_hOpenFile(buffer, "wb");
	if (!f) {
		fb_hStrDelTemp(filename);
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum( FB_RTERROR_FILENOTFOUND );
	}

	fb_hPrepareTarget(context, NULL);
	fb_hSetPixelTransfer(context, MASK_A_32);

	p = strrchr(filename->data, '.');
	if (p && (!strcasecmp(p + 1, "bmp"))) {
		result = save_bmp(context, f, src, pal, bitsperpixel);
	} else if (p && (!strcasecmp(p + 1, "png"))) {
		result = fb_hGfxPngSave(context, f, src, pal, bitsperpixel);
	} else {
		if ((size == 0) && src) {
			fclose(f);
			fb_hStrDelTemp(filename);
			FB_GRAPHICS_UNLOCK( );
			return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
		}

		fputc(0xFE, f);
		fputc(size & 0xFF, f);
		fputc((size >> 8) & 0xFF, f);
		fputc((size >> 16) & 0xFF, f);
		fputc((int)((size >> 24) & 0xFFu), f);

		if (!src) {
			DRIVER_LOCK();
			size = MIN(size, (unsigned int)(fb_gfx->pitch * fb_gfx->h));
			if (!fwrite(context->line[0], size, 1, f))
				result = FB_RTERROR_FILEIO;
			DRIVER_UNLOCK();
			if (fb_gfx->depth <= 8) {
				for (i = 0; i < (1 << fb_gfx->depth); i++) {
					if (pal)
						color = palette[i];
					else
						color = (fb_gfx->device_palette[i] >> 2) & 0x3F3F3F;
					fputc(color & 0xFF, f);
					fputc((color >> 8) & 0xFF, f);
					fputc((color >> 16) & 0xFF, f);
				}
			}
		}
		else if (!fwrite(src, size, 1, f))
			result = FB_RTERROR_FILEIO;
	}

	fclose(f);

	fb_hStrDelTemp(filename);
	FB_GRAPHICS_UNLOCK( );
	return fb_ErrorSetNum( result );
}

FBCALL int fb_GfxBsave(FBSTRING_ACCESS *filename, void *src, unsigned int size, void *pal)
{
	return fb_GfxBsaveEx(filename, src, size, pal, 0);
}

/* end of gfx_bsave.c */
