/* TRANS drawing method for PUT statement */

#include "fb_gfx.h"
#include "gfx_simd.h"

#ifdef HOST_X86
#include "x86/fb_gfx_mmx.h"
extern void fb_hPutTrans1MMX(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param);
extern void fb_hPutTrans2MMX(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param);
extern void fb_hPutTrans4MMX(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param);
#endif

void fb_hPutTrans1C(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	unsigned char *s = (unsigned char *)src;
	unsigned char *d;
	int x;
	
	src_pitch -= w;
	for (; h; h--) {
		d = (unsigned char *)dest;
		for (x = w; x; x--) {
			if (*s)
				*d = (unsigned int)*s;
			s++;
			d++;
		}
		s += src_pitch;
		dest += dest_pitch;
	}
}

static void fb_hPutTrans2C(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	unsigned short *s = (unsigned short *)src;
	unsigned short *d;
	int x;
	
	src_pitch = (src_pitch >> 1) - w;
	for (; h; h--) {
		d = (unsigned short *)dest;
		for (x = w; x; x--) {
			if (*s != MASK_COLOR_16)
				*d = (unsigned short)*s;
			s++;
			d++;
		}
		s += src_pitch;
		dest += dest_pitch;
	}
}

static void fb_hPutTrans4C(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	unsigned int *s = (unsigned int *)src;
	unsigned int *d, c;
	int x;
	
	src_pitch = (src_pitch >> 2) - w;
	for (; h; h--) {
		d = (unsigned int *)dest;
		for (x = w; x; x--) {
			c = *s & 0x00FFFFFF;
			if (c != MASK_COLOR_32)
				*d = c;
			s++;
			d++;
		}
		s += src_pitch;
		dest += dest_pitch;
	}
}

/* Not thread-safe; putters should only be called from other gfx functions that
   take care of the synchronization */
void fb_hPutTrans(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	static PUTTER *c_putters[] = {
		fb_hPutTrans1C, fb_hPutTrans2C, NULL, fb_hPutTrans4C,
	};
#ifdef FB_GFX_HAS_SIMD
	static PUTTER *simd_putters[] = {
		fb_hPutTrans1SIMD, fb_hPutTrans2SIMD, NULL, fb_hPutTrans4SIMD,
	};
#endif
#ifdef HOST_X86
	static PUTTER *mmx_putters[] = {
		fb_hPutTrans1MMX, fb_hPutTrans2MMX, NULL, fb_hPutTrans4MMX,
	};
#endif
	PUTTER *putter;
	FB_GFXCTX *context = fb_hGetContext();
	
	if (!context->putter[PUT_MODE_TRANS]) {
#ifdef FB_GFX_HAS_SIMD
		if (fb_hSimdAvailable())
			context->putter[PUT_MODE_TRANS] = simd_putters;
		else
#endif
#ifdef HOST_X86
		if (__fb_gfx->flags & X86_MMX_ENABLED)
			context->putter[PUT_MODE_TRANS] = mmx_putters;
		else
#endif
			context->putter[PUT_MODE_TRANS] = c_putters;
	}
	putter = context->putter[PUT_MODE_TRANS][context->target_bpp - 1];
	
	putter(src, dest, w, h, src_pitch, dest_pitch, alpha, blender, param);
}
