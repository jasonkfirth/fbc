/*
 * FreeBASIC gfxlib2: gfx_image_convert.c
 * Purpose: Convert image rows between supported pixel formats.
 * Responsibilities: image storage, conversion and checked file operations.
 * This file intentionally does NOT contain native windows or input dispatch.
 */

/* bpp conversion */

#include "../fb_gfx.h"

/*
    Conversion destinations are byte-addressed and may point into packed rows.
    These aliases keep scalar pixel stores valid when the address is unaligned.
*/
#if defined(__GNUC__) || defined(__clang__)
typedef unsigned short fb_image_u16_unaligned
	__attribute__((__aligned__(1), __may_alias__));
typedef unsigned int fb_image_u32_unaligned
	__attribute__((__aligned__(1), __may_alias__));
#else
typedef unsigned short fb_image_u16_unaligned;
typedef unsigned int fb_image_u32_unaligned;
#endif

/* Caller is expected to hold FB_GRAPHICS_LOCK() */
void fb_image_convert_8to8(const unsigned char *src, unsigned char *dest, int w)
{
	for (; w > 0; w--)
		*dest++ = (unsigned char)(*src++ & fb_gfx->color_mask);
}

/* Caller is expected to hold FB_GRAPHICS_LOCK() */
void fb_image_convert_8to16(const unsigned char *src, unsigned char *dest, int w)
{
	int r, g, b;
	fb_image_u16_unaligned *d = (fb_image_u16_unaligned *)dest;

	for (; w > 0; w--) {
		r = fb_gfx->device_palette[*src] & 0xFF;
		g = (fb_gfx->device_palette[*src] >> 8) & 0xFF;
		b = (fb_gfx->device_palette[*src] >> 16) & 0xFF;
		*d++ = (fb_image_u16_unaligned)((b >> 3) | ((g << 3) & 0x07E0) | ((r << 8) & 0xF800));
		src++;
	}
}

/* Caller is expected to hold FB_GRAPHICS_LOCK() */
void fb_image_convert_8to32(const unsigned char *src, unsigned char *dest, int w)
{
	unsigned int r, g, b;
	fb_image_u32_unaligned *d = (fb_image_u32_unaligned *)dest;

	for (; w > 0; w--) {
		r = fb_gfx->device_palette[*src] & 0xFFu;
		g = (fb_gfx->device_palette[*src] >> 8) & 0xFFu;
		b = (fb_gfx->device_palette[*src] >> 16) & 0xFFu;
		*d++ = 0xFF000000u | b | (g << 8) | (r << 16);
		src++;
	}
}

void fb_image_convert_24to16(const unsigned char *src, unsigned char *dest, int w)
{
	fb_image_u16_unaligned *d = (fb_image_u16_unaligned *)dest;
	for (; w > 0; w--) {
		*d++ = (fb_image_u16_unaligned)((((unsigned short)src[0] << 8) & 0xF800) | (((unsigned short)src[1] << 3) & 0x07E0) | ((unsigned short)src[2] >> 3));
		src += 3;
	}
}


void fb_image_convert_24to32(const unsigned char *src, unsigned char *dest, int w)
{
	fb_image_u32_unaligned *d = (fb_image_u32_unaligned *)dest;

	for (; w > 0; w--) {
		*d++ = 0xFF000000 | ((unsigned int)src[0] << 16) | ((unsigned int)src[1] << 8) | ((unsigned int)src[2]);
		src += 3;
	}
}

void fb_image_convert_32to16(const unsigned char *src, unsigned char *dest, int w)
{
	fb_image_u16_unaligned *d = (fb_image_u16_unaligned *)dest;

	for (; w > 0; w--)
	{
		*d++ = (unsigned short)((((unsigned short)src[0] << 8) & 0xF800) |
		                        (((unsigned short)src[1] << 3) & 0x07E0) |
		                        ((unsigned short)src[2] >> 3));
		src += sizeof( unsigned int );
	}
}

void fb_image_convert_32to32(const unsigned char *src, unsigned char *dest, int w)
{
	fb_image_u32_unaligned *d = (fb_image_u32_unaligned *)dest;

	for (; w > 0; w--)
	{
		*d++ = ((unsigned int)src[3] << 24) |
		       ((unsigned int)src[0] << 16) |
		       ((unsigned int)src[1] << 8) |
		       ((unsigned int)src[2]);
		src += sizeof( unsigned int );
	}
}

void fb_image_convert_24bgrto16(const unsigned char *src, unsigned char *dest, int w)
{
	fb_image_u16_unaligned *d = (fb_image_u16_unaligned *)dest;
	for (; w > 0; w--) {
		*d++ = (fb_image_u16_unaligned)(((unsigned short)src[0] >> 3) | (((unsigned short)src[1] << 3) & 0x07E0) | (((unsigned short)src[2] << 8) & 0xF800));
		src += 3;
	}
}

void fb_image_convert_24bgrto32(const unsigned char *src, unsigned char *dest, int w)
{
	fb_image_u32_unaligned *d = (fb_image_u32_unaligned *)dest;

	for (; w > 0; w--) {
		*d++ = 0xFF000000 | ((unsigned int)src[2] << 16) | ((unsigned int)src[1] << 8) | ((unsigned int)src[0]);
		src += 3;
	}
}

void fb_image_convert_32bgrto16(const unsigned char *src, unsigned char *dest, int w)
{
	fb_image_u16_unaligned *d = (fb_image_u16_unaligned *)dest;

	for (; w > 0; w--) {
		*d++ = (unsigned short)(((unsigned short)src[0] >> 3) |
		                        (((unsigned short)src[1] << 3) & 0x07E0) |
		                        (((unsigned short)src[2] << 8) & 0xF800));
		src += sizeof( unsigned int );
	}
}

void fb_image_convert_32bgrto32(const unsigned char *src, unsigned char *dest, int w)
{
	fb_image_u32_unaligned *d = (fb_image_u32_unaligned *)dest;

	for (; w > 0; w--) {
		*d++ = ((unsigned int)src[3] << 24) |
		       ((unsigned int)src[2] << 16) |
		       ((unsigned int)src[1] << 8) |
		       ((unsigned int)src[0]);
		src += sizeof( unsigned int );
	}
}

static int fb_hImageBytesPerPixel(int depth)
{
	if (depth <= 0)
		return 0;
	return (depth / 8) + ((depth % 8) != 0);
}

FBCALL void fb_GfxImageConvertRow( const unsigned char *src, int src_bpp,
								   unsigned char *dest, int dst_bpp,
								   int width, int isrgb )
{
	FBGFX_IMAGE_CONVERT convert = NULL;

	if ((width < 0) ||
	    ((width > 0) && ((src == NULL) || (dest == NULL))))
		return;

	FB_GRAPHICS_LOCK( );

	if (src_bpp <= 8)
	{
		switch (fb_hImageBytesPerPixel(dst_bpp))
		{
			case 1: convert = fb_image_convert_8to8;  break;
			case 2: convert = fb_image_convert_8to16; break;
			case 3:
			case 4: convert = fb_image_convert_8to32; break;
			default: break;
		}
	}
	else if (src_bpp == 24)
	{
		switch (fb_hImageBytesPerPixel(dst_bpp))
		{
			case 1: break;
			case 2: convert = (isrgb != 0? fb_image_convert_24to16: fb_image_convert_24bgrto16); break;
			case 3:
			case 4: convert = (isrgb != 0? fb_image_convert_24to32: fb_image_convert_24bgrto32); break;
			default: break;
		}
	}
	else if (src_bpp == 32)
	{
		switch (fb_hImageBytesPerPixel(dst_bpp))
		{
			case 1: break;
			case 2: convert = (isrgb != 0? fb_image_convert_32to16: fb_image_convert_32bgrto16); break;
			case 3:
			case 4: convert = (isrgb != 0? fb_image_convert_32to32: fb_image_convert_32bgrto32); break;
			default: break;
		}
	}

	if( convert ) {
		convert( src, dest, width );
	}
	FB_GRAPHICS_UNLOCK( );
}

/* end of gfx_image_convert.c */
