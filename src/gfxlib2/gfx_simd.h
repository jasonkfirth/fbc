/*
    Project: FreeBASIC gfxlib2
    --------------------------

    File: gfx_simd.h

    Purpose:

        Define the private contract for architecture-specific SIMD drawing
        kernels.

    Responsibilities:

        - identify targets with a gfxlib2 SIMD implementation
        - declare accelerated pixel fills and PUT compositing operations
        - expose the ARM runtime feature decision through one checked helper

    This file intentionally does NOT contain:

        - instruction-set intrinsics
        - 32-bit x86 MMX declarations or controls
        - public FreeBASIC declarations
*/

#ifndef __FB_GFX_SIMD_H__
#define __FB_GFX_SIMD_H__

#include "fb_gfx.h"

/*
    SIMD baselines

    The x86_64 ABI requires SSE2 and AArch64 requires Advanced SIMD.  ARMv7
    does not require NEON, so its separately compiled kernels are selected
    after a runtime hardware-capability check.
*/
#if defined(HOST_X86_64) || defined(__aarch64__) || \
	defined(FB_GFX_ARM_NEON)
#define FB_GFX_HAS_SIMD
#endif

#ifdef FB_GFX_HAS_SIMD

int fb_hSimdAvailable(void);

void *fb_hPixelSet2SIMD(void *dest, int color, size_t size);
void *fb_hPixelSet4SIMD(void *dest, int color, size_t size);
void *fb_hPixelSetAlpha4SIMD(void *dest, int color, size_t size);

void fb_hPutAlphaMaskSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutAndSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutOrSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutXorSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutPResetSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutTrans1SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutTrans2SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutTrans4SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutAlpha4SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutAdd4SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutBlend2SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);
void fb_hPutBlend4SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param);

#endif

#endif

/* end of gfx_simd.h */
