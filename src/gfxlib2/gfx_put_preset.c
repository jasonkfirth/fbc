/* PRESET drawing method for PUT statement */

#include "fb_gfx.h"
#include "gfx_simd.h"

#ifdef HOST_X86
#include "x86/fb_gfx_mmx.h"
extern void fb_hPutPResetMMX(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param);
#endif

static void fb_hPutPResetC(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	int x;
	FB_GFXCTX *context = fb_hGetContext();
	
	w <<= (context->target_bpp >> 1);
	src_pitch -= w;
	dest_pitch -= w;
	for (; h; h--) {
		if (w & 1)
			*dest++ = 0xFF ^ *src++;
		if (w & 2) {
			*(unsigned short *)dest = 0xFFFF ^ *(unsigned short *)src;
			dest += 2;
			src += 2;
		}
		for (x = w >> 2; x; x--) {
			*(unsigned int *)dest = 0xFFFFFFFF ^ *(unsigned int *)src;
			dest += 4;
			src += 4;
		}
		src += src_pitch;
		dest += dest_pitch;
	}
}

/* Not thread-safe; putters should only be called from other gfx functions that
   take care of the synchronization */
void fb_hPutPReset(unsigned char *src, unsigned char *dest, int w, int h, int src_pitch, int dest_pitch, int alpha, BLENDER *blender, void *param)
{
	static PUTTER *c_putters[] = {
		fb_hPutPResetC, fb_hPutPResetC, NULL, fb_hPutPResetC,
	};
#ifdef FB_GFX_HAS_SIMD
	static PUTTER *simd_putters[] = {
		fb_hPutPResetSIMD, fb_hPutPResetSIMD, NULL, fb_hPutPResetSIMD,
	};
#endif
#ifdef HOST_X86
	static PUTTER *mmx_putters[] = {
		fb_hPutPResetMMX, fb_hPutPResetMMX, NULL, fb_hPutPResetMMX,
	};
#endif
	PUTTER *putter;
	FB_GFXCTX *context = fb_hGetContext();
	
	if (!context->putter[PUT_MODE_PRESET]) {
#ifdef FB_GFX_HAS_SIMD
		if (fb_hSimdAvailable())
			context->putter[PUT_MODE_PRESET] = simd_putters;
		else
#endif
#ifdef HOST_X86
		if (__fb_gfx->flags & X86_MMX_ENABLED)
			context->putter[PUT_MODE_PRESET] = mmx_putters;
		else
#endif
			context->putter[PUT_MODE_PRESET] = c_putters;
	}
	putter = context->putter[PUT_MODE_PRESET][context->target_bpp - 1];
	
	putter(src, dest, w, h, src_pitch, dest_pitch, alpha, blender, param);
}
