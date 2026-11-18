/*
 * FreeBASIC gfxlib2: gfx_put_custom.c
 * Purpose: Implement the CUSTOM image PUT operator.
 * Responsibilities: pixel formats, PUT modes and optimized transfer selection.
 * This file intentionally does NOT contain window creation or input state.
 */

/* CUSTOM drawing method for PUT statement */

#include "../fb_gfx.h"

/* Byte-pitched rows can leave packed pixels at unaligned addresses. */
#if defined(__GNUC__) || defined(__clang__)
typedef unsigned short fb_put_u16_unaligned
	__attribute__((__aligned__(1), __may_alias__));
typedef unsigned int fb_put_u32_unaligned
	__attribute__((__aligned__(1), __may_alias__));
#else
typedef unsigned short fb_put_u16_unaligned;
typedef unsigned int fb_put_u32_unaligned;
#endif

static void fb_hPutCustom1(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	unsigned char *s = (unsigned char *)src;
	unsigned char *d;
	int x;
	(void)alpha;

	src_pitch -= w;
	for (; h; h--) {
		d = (unsigned char *)dest;
		for (x = w; x; x--) {
			*d = (unsigned char)blender((unsigned int)*s, (unsigned int)*d, param);
			s++;
			d++;
		}
		s += src_pitch;
		dest += dest_pitch;
	}
}

static void fb_hPutCustom2(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	fb_put_u16_unaligned *s = (fb_put_u16_unaligned *)src;
	fb_put_u16_unaligned *d;
	int x;
	unsigned int cs, cd;
	(void)alpha;

	src_pitch = (src_pitch >> 1) - w;
	for (; h; h--) {
		d = (fb_put_u16_unaligned *)dest;
		for (x = w; x; x--) {
			cs = *s++;
			cs = (((cs & 0x001F) << 3) | ((cs >> 2) & 0x7) |
			      ((cs & 0x07E0) << 5) | ((cs >> 1) & 0x300) |
			      ((cs & 0xF800) << 8) | ((cs << 3) & 0x70000));
			cd = *d;
			cd = (((cd & 0x001F) << 3) | ((cd >> 2) & 0x7) |
			      ((cd & 0x07E0) << 5) | ((cd >> 1) & 0x300) |
			      ((cd & 0xF800) << 8) | ((cd << 3) & 0x70000));
			cd = blender(cs, cd, param);
			*d++ = (fb_put_u16_unaligned)(((cd >> 3) & 0x001F) |
				((cd >> 5) & 0x07E0) | ((cd >> 8) & 0xF800));
		}
		s += src_pitch;
		dest += dest_pitch;
	}
}

static void fb_hPutCustom4(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	fb_put_u32_unaligned *s = (fb_put_u32_unaligned *)src;
	fb_put_u32_unaligned *d;
	int x;
	unsigned int source_pixel, destination_pixel;
	(void)alpha;
	
	src_pitch = (src_pitch >> 2) - w;
	for (; h; h--) {
		d = (fb_put_u32_unaligned *)dest;
		for (x = w; x; x--) {
			source_pixel = *s;
			destination_pixel = *d;
			*d = blender(source_pixel, destination_pixel, param);
			s++;
			d++;
		}
		s += src_pitch;
		dest += dest_pitch;
	}
}

/* Not thread-safe; putters should only be called from other gfx functions that
   take care of the synchronization */
void fb_hPutCustom(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	static PUTTER *all_putters[] = {
		fb_hPutCustom1, fb_hPutCustom2, NULL, fb_hPutCustom4,
	};
	PUTTER *putter;
	FB_GFXCTX *context = fb_hGetContext();
	
	if (!context->putter[PUT_MODE_CUSTOM]) {
		context->putter[PUT_MODE_CUSTOM] = &all_putters[0];
	}
	putter = context->putter[PUT_MODE_CUSTOM][context->target_bpp - 1];
	
	putter(src, dest, w, h, src_pitch, dest_pitch, alpha, blender, param);
}

/* end of gfx_put_custom.c */
