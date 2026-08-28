/*
    Project: FreeBASIC gfxlib2 NEON kernels
    ---------------------------------------

    File: gfx_simd_neon.inc

    Purpose:

        Implement drawing kernels shared by the ARM32 and AArch64 NEON entry
        points.

    Responsibilities:

        - fill 16-bit and 32-bit pixel spans
        - blend alpha-bearing 32-bit pixels with exact integer arithmetic
        - accelerate logical, transparent, additive, and blended PUT modes
        - handle arbitrary row alignment and scalar tails safely

    This file intentionally does NOT contain:

        - architecture feature detection
        - build-system instruction-set flags
        - framebuffer locking or dirty-line management

    Inclusion contract:

        The including source must include gfx_simd.h and arm_neon.h first.
*/

#ifndef __FB_GFX_SIMD_NEON_INC__
#define __FB_GFX_SIMD_NEON_INC__

enum FB_GFX_SIMD_LOGIC {
	FB_GFX_SIMD_AND,
	FB_GFX_SIMD_OR,
	FB_GFX_SIMD_XOR,
	FB_GFX_SIMD_PRESET
};

/* All 32-bit gfxlib2 drawing colours store RGB in the low 24 bits. */
#define FB_GFX_SIMD_RGB_MASK_32 0x00FFFFFFu

/* ------------------------------------------------------------------------- */
/* Shared NEON helpers                                                       */
/* ------------------------------------------------------------------------- */

static void simd_expand_source_alpha(uint8x16_t source, int add_one,
	uint16x8_t *alpha_low, uint16x8_t *alpha_high)
{
	uint32x4_t alpha_32;
	uint16x4_t alpha_16;
	uint16x4x2_t pairs;
	uint16x4x2_t first_two;
	uint16x4x2_t last_two;

	alpha_32 = vshrq_n_u32(vreinterpretq_u32_u8(source), 24);
	alpha_16 = vmovn_u32(alpha_32);
	if (add_one)
		alpha_16 = vadd_u16(alpha_16, vdup_n_u16(1u));
	pairs = vzip_u16(alpha_16, alpha_16);
	first_two = vzip_u16(pairs.val[0], pairs.val[0]);
	last_two = vzip_u16(pairs.val[1], pairs.val[1]);
	*alpha_low = vcombine_u16(first_two.val[0], first_two.val[1]);
	*alpha_high = vcombine_u16(last_two.val[0], last_two.val[1]);
}

static uint8x16_t simd_blend_factors(uint8x16_t source,
	uint8x16_t destination, uint16x8_t alpha_low,
	uint16x8_t alpha_high)
{
	const uint16x8_t full = vdupq_n_u16(256u);
	uint16x8_t source_low = vmovl_u8(vget_low_u8(source));
	uint16x8_t source_high = vmovl_u8(vget_high_u8(source));
	uint16x8_t destination_low = vmovl_u8(vget_low_u8(destination));
	uint16x8_t destination_high = vmovl_u8(vget_high_u8(destination));
	uint16x8_t inverse_low = vsubq_u16(full, alpha_low);
	uint16x8_t inverse_high = vsubq_u16(full, alpha_high);
	uint16x8_t result_low;
	uint16x8_t result_high;

	result_low = vaddq_u16(vmulq_u16(source_low, alpha_low),
		vmulq_u16(destination_low, inverse_low));
	result_high = vaddq_u16(vmulq_u16(source_high, alpha_high),
		vmulq_u16(destination_high, inverse_high));
	result_low = vshrq_n_u16(result_low, 8);
	result_high = vshrq_n_u16(result_high, 8);
	return vcombine_u8(vmovn_u16(result_low), vmovn_u16(result_high));
}

static uint8x16_t simd_blend_source_alpha(uint8x16_t source,
	uint8x16_t destination, int add_one, int preserve_source_alpha)
{
	const uint8x16_t alpha_mask = vreinterpretq_u8_u32(
		vdupq_n_u32(MASK_A_32));
	uint16x8_t alpha_low;
	uint16x8_t alpha_high;
	uint8x16_t result;

	simd_expand_source_alpha(source, add_one, &alpha_low, &alpha_high);
	result = simd_blend_factors(source, destination, alpha_low, alpha_high);
	if (preserve_source_alpha)
		result = vbslq_u8(alpha_mask, source, result);
	return result;
}

static uint8x16_t simd_blend_constant(uint8x16_t source,
	uint8x16_t destination, unsigned int alpha)
{
	const uint16x8_t factor = vdupq_n_u16((uint16_t)alpha);

	return simd_blend_factors(source, destination, factor, factor);
}

static uint16x8_t simd_blend_rgb565(uint16x8_t source,
	uint16x8_t destination, int16x8_t factor)
{
	const uint16x8_t mask_5 = vdupq_n_u16(0x1Fu);
	const uint16x8_t mask_6 = vdupq_n_u16(0x3Fu);
	const uint16x8_t transparent_color = vdupq_n_u16(MASK_COLOR_16);
	uint16x8_t source_r = vandq_u16(vshrq_n_u16(source, 11), mask_5);
	uint16x8_t source_g = vandq_u16(vshrq_n_u16(source, 5), mask_6);
	uint16x8_t source_b = vandq_u16(source, mask_5);
	uint16x8_t destination_r = vandq_u16(
		vshrq_n_u16(destination, 11), mask_5);
	uint16x8_t destination_g = vandq_u16(
		vshrq_n_u16(destination, 5), mask_6);
	uint16x8_t destination_b = vandq_u16(destination, mask_5);
	int16x8_t result_r;
	int16x8_t result_g;
	int16x8_t result_b;
	uint16x8_t result;
	uint16x8_t transparent;

	result_r = vshrq_n_s16(vmulq_s16(vreinterpretq_s16_u16(
		vsubq_u16(source_r, destination_r)), factor), 5);
	result_g = vshrq_n_s16(vmulq_s16(vreinterpretq_s16_u16(
		vsubq_u16(source_g, destination_g)), factor), 5);
	result_b = vshrq_n_s16(vmulq_s16(vreinterpretq_s16_u16(
		vsubq_u16(source_b, destination_b)), factor), 5);
	result_r = vaddq_s16(vreinterpretq_s16_u16(destination_r), result_r);
	result_g = vaddq_s16(vreinterpretq_s16_u16(destination_g), result_g);
	result_b = vaddq_s16(vreinterpretq_s16_u16(destination_b), result_b);
	result = vorrq_u16(vshlq_n_u16(vreinterpretq_u16_s16(result_r), 11),
		vorrq_u16(vshlq_n_u16(vreinterpretq_u16_s16(result_g), 5),
			vreinterpretq_u16_s16(result_b)));
	transparent = vceqq_u16(source, transparent_color);
	return vbslq_u16(transparent, destination, result);
}

/* ------------------------------------------------------------------------- */
/* Pixel fills                                                               */
/* ------------------------------------------------------------------------- */

void *fb_hPixelSet2SIMD(void *dest, int color, size_t size)
{
	unsigned short *cursor = (unsigned short *)dest;
	const uint16x8_t fill = vdupq_n_u16((uint16_t)color);

	while (size >= 8u) {
		vst1q_u16(cursor, fill);
		cursor += 8;
		size -= 8u;
	}
	while (size != 0u) {
		*cursor++ = (unsigned short)color;
		--size;
	}
	return dest;
}

void *fb_hPixelSet4SIMD(void *dest, int color, size_t size)
{
	unsigned int *cursor = (unsigned int *)dest;
	const uint32x4_t fill = vdupq_n_u32((uint32_t)color);

	while (size >= 4u) {
		vst1q_u32(cursor, fill);
		cursor += 4;
		size -= 4u;
	}
	while (size != 0u) {
		*cursor++ = (unsigned int)color;
		--size;
	}
	return dest;
}

void *fb_hPixelSetAlpha4SIMD(void *dest, int color, size_t size)
{
	unsigned int *cursor = (unsigned int *)dest;
	const uint8x16_t source = vreinterpretq_u8_u32(
		vdupq_n_u32((uint32_t)color));
	unsigned int source_color = (unsigned int)color;
	unsigned int source_rb = source_color & MASK_RB_32;
	unsigned int source_g = source_color & MASK_G_32;
	unsigned int source_a = source_color & MASK_A_32;
	unsigned int alpha = source_color >> 24;

	while (size >= 4u) {
		uint8x16_t destination = vld1q_u8((const uint8_t *)cursor);
		uint8x16_t result = simd_blend_source_alpha(source, destination,
			FALSE, TRUE);

		vst1q_u8((uint8_t *)cursor, result);
		cursor += 4;
		size -= 4u;
	}
	while (size != 0u) {
		unsigned int destination = *cursor;
		unsigned int destination_rb = destination & MASK_RB_32;
		unsigned int destination_g = destination & MASK_G_32;
		unsigned int result_rb;
		unsigned int result_g;

		result_rb = ((source_rb - destination_rb) * alpha) >> 8;
		result_g = ((source_g - destination_g) * alpha) >> 8;
		*cursor++ = ((destination_rb + result_rb) & MASK_RB_32) |
			((destination_g + result_g) & MASK_G_32) | source_a;
		--size;
	}
	return dest;
}

/* ------------------------------------------------------------------------- */
/* Logical PUT modes                                                         */
/* ------------------------------------------------------------------------- */

static void simd_logic_row(unsigned char *source,
	unsigned char *destination, size_t bytes, enum FB_GFX_SIMD_LOGIC operation)
{
	size_t offset = 0u;

	while ((bytes - offset) >= 16u) {
		uint8x16_t source_pixels = vld1q_u8(source + offset);
		uint8x16_t destination_pixels = vld1q_u8(destination + offset);
		uint8x16_t result;

		switch (operation) {
		case FB_GFX_SIMD_AND:
			result = vandq_u8(destination_pixels, source_pixels);
			break;
		case FB_GFX_SIMD_OR:
			result = vorrq_u8(destination_pixels, source_pixels);
			break;
		case FB_GFX_SIMD_XOR:
			result = veorq_u8(destination_pixels, source_pixels);
			break;
		default:
			result = vmvnq_u8(source_pixels);
			break;
		}
		vst1q_u8(destination + offset, result);
		offset += 16u;
	}
	while (offset < bytes) {
		switch (operation) {
		case FB_GFX_SIMD_AND:
			destination[offset] &= source[offset];
			break;
		case FB_GFX_SIMD_OR:
			destination[offset] |= source[offset];
			break;
		case FB_GFX_SIMD_XOR:
			destination[offset] ^= source[offset];
			break;
		default:
			destination[offset] = (unsigned char)~source[offset];
			break;
		}
		++offset;
	}
}

static void simd_put_logic(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch,
	enum FB_GFX_SIMD_LOGIC operation)
{
	FB_GFXCTX *context;
	size_t row_bytes;

	if ((w <= 0) || (h <= 0))
		return;
	context = fb_hGetContext();
	row_bytes = (size_t)w << (context->target_bpp >> 1);
	while (h-- != 0) {
		simd_logic_row(src, dest, row_bytes, operation);
		src += src_pitch;
		dest += dest_pitch;
	}
}

void fb_hPutAndSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	simd_put_logic(src, dest, w, h, src_pitch, dest_pitch,
		FB_GFX_SIMD_AND);
}

void fb_hPutOrSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	simd_put_logic(src, dest, w, h, src_pitch, dest_pitch,
		FB_GFX_SIMD_OR);
}

void fb_hPutXorSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	simd_put_logic(src, dest, w, h, src_pitch, dest_pitch,
		FB_GFX_SIMD_XOR);
}

void fb_hPutPResetSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	simd_put_logic(src, dest, w, h, src_pitch, dest_pitch,
		FB_GFX_SIMD_PRESET);
}

/* ------------------------------------------------------------------------- */
/* Transparent PUT modes                                                     */
/* ------------------------------------------------------------------------- */

void fb_hPutTrans1SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	const uint8x16_t zero = vdupq_n_u8(0u);

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		int x = 0;

		for (; x <= (w - 16); x += 16) {
			uint8x16_t source = vld1q_u8(src + x);
			uint8x16_t destination = vld1q_u8(dest + x);
			uint8x16_t transparent = vceqq_u8(source, zero);

			vst1q_u8(dest + x,
				vbslq_u8(transparent, destination, source));
		}
		for (; x < w; ++x) {
			if (src[x] != 0u)
				dest[x] = src[x];
		}
		src += src_pitch;
		dest += dest_pitch;
	}
}

void fb_hPutTrans2SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	const uint16x8_t transparent_color = vdupq_n_u16(MASK_COLOR_16);

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned short *source = (unsigned short *)src;
		unsigned short *destination = (unsigned short *)dest;
		int x = 0;

		for (; x <= (w - 8); x += 8) {
			uint16x8_t source_pixels = vld1q_u16(source + x);
			uint16x8_t destination_pixels = vld1q_u16(
				destination + x);
			uint16x8_t transparent = vceqq_u16(source_pixels,
				transparent_color);

			vst1q_u16(destination + x,
				vbslq_u16(transparent, destination_pixels,
					source_pixels));
		}
		for (; x < w; ++x) {
			if (source[x] != MASK_COLOR_16)
				destination[x] = source[x];
		}
		src += src_pitch;
		dest += dest_pitch;
	}
}

void fb_hPutTrans4SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	const uint32x4_t rgb_mask = vdupq_n_u32(FB_GFX_SIMD_RGB_MASK_32);
	const uint32x4_t transparent_color = vdupq_n_u32(MASK_COLOR_32);

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned int *source = (unsigned int *)src;
		unsigned int *destination = (unsigned int *)dest;
		int x = 0;

		for (; x <= (w - 4); x += 4) {
			uint32x4_t source_pixels = vld1q_u32(source + x);
			uint32x4_t destination_pixels = vld1q_u32(
				destination + x);
			uint32x4_t colors = vandq_u32(source_pixels, rgb_mask);
			uint32x4_t transparent = vceqq_u32(colors,
				transparent_color);

			vst1q_u32(destination + x,
				vbslq_u32(transparent, destination_pixels, colors));
		}
		for (; x < w; ++x) {
			unsigned int color = source[x] & FB_GFX_SIMD_RGB_MASK_32;

			if (color != MASK_COLOR_32)
				destination[x] = color;
		}
		src += src_pitch;
		dest += dest_pitch;
	}
}

/* ------------------------------------------------------------------------- */
/* Alpha, additive, and constant blend PUT modes                             */
/* ------------------------------------------------------------------------- */

void fb_hPutAlpha4SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned int *source = (unsigned int *)src;
		unsigned int *destination = (unsigned int *)dest;
		int x = 0;

		for (; x <= (w - 4); x += 4) {
			uint8x16_t source_pixels = vld1q_u8(
				(const uint8_t *)(source + x));
			uint8x16_t destination_pixels = vld1q_u8(
				(const uint8_t *)(destination + x));
			uint8x16_t result = simd_blend_source_alpha(source_pixels,
				destination_pixels, TRUE, FALSE);

			vst1q_u8((uint8_t *)(destination + x), result);
		}
		for (; x < w; ++x) {
			unsigned int source_color = source[x];
			unsigned int destination_color = destination[x];
			unsigned int factor = (source_color >> 24) + 1u;
			unsigned int source_rb = source_color & MASK_RB_32;
			unsigned int source_ga = source_color & MASK_GA_32;
			unsigned int destination_rb = destination_color & MASK_RB_32;
			unsigned int destination_ga = destination_color & MASK_GA_32;

			source_rb = ((source_rb - destination_rb) * factor) >> 8;
			source_ga = ((source_ga >> 8) - (destination_ga >> 8)) *
				factor;
			destination[x] =
				((destination_rb + source_rb) & MASK_RB_32) |
				((destination_ga + source_ga) & MASK_GA_32);
		}
		src += src_pitch;
		dest += dest_pitch;
	}
}

void fb_hPutAdd4SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	const uint16x8_t factor = vdupq_n_u16((uint16_t)(alpha & 0xFF));
	const uint32x4_t rgb_mask = vdupq_n_u32(FB_GFX_SIMD_RGB_MASK_32);
	const uint32x4_t transparent_color = vdupq_n_u32(MASK_COLOR_32);
	unsigned int amount = (unsigned int)alpha & 0xFFu;

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned int *source = (unsigned int *)src;
		unsigned int *destination = (unsigned int *)dest;
		int x = 0;

		for (; x <= (w - 4); x += 4) {
			uint8x16_t source_pixels = vld1q_u8(
				(const uint8_t *)(source + x));
			uint8x16_t destination_pixels = vld1q_u8(
				(const uint8_t *)(destination + x));
			uint16x8_t source_low = vmovl_u8(
				vget_low_u8(source_pixels));
			uint16x8_t source_high = vmovl_u8(
				vget_high_u8(source_pixels));
			uint8x16_t scaled;
			uint8x16_t result;
			uint32x4_t transparent;

			source_low = vshrq_n_u16(vmulq_u16(source_low, factor), 8);
			source_high = vshrq_n_u16(vmulq_u16(source_high, factor), 8);
			scaled = vcombine_u8(vmovn_u16(source_low),
				vmovn_u16(source_high));
			result = vqaddq_u8(scaled, destination_pixels);
			transparent = vceqq_u32(vandq_u32(
				vreinterpretq_u32_u8(source_pixels), rgb_mask),
				transparent_color);
			result = vbslq_u8(vreinterpretq_u8_u32(transparent),
				destination_pixels, result);
			vst1q_u8((uint8_t *)(destination + x), result);
		}
		for (; x < w; ++x) {
			unsigned int source_color = source[x];
			unsigned int destination_color = destination[x];
			unsigned int first;
			unsigned int second;

			if ((source_color & FB_GFX_SIMD_RGB_MASK_32) == MASK_COLOR_32)
				continue;
			first = source_color & MASK_RB_32;
			second = (source_color >> 8) & MASK_RB_32;
			first = ((first * amount) >> 8) & MASK_RB_32;
			second = (second * amount) & MASK_GA_32;
			source_color = first | second;
			first = source_color & 0x80808080u;
			second = destination_color & 0x80808080u;
			source_color = (source_color & 0x7F7F7F7Fu) +
				(destination_color & 0x7F7F7F7Fu);
			destination_color = first;
			first |= second;
			second &= destination_color;
			destination_color = first & source_color;
			source_color |= ((((second | destination_color) >> 7) +
				0x7F7F7F7Fu) ^ 0x7F7F7F7Fu) | first;
			destination[x] = source_color;
		}
		src += src_pitch;
		dest += dest_pitch;
	}
}

void fb_hPutBlend2SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	unsigned int amount = ((unsigned int)(alpha & 0xFF) + 7u) >> 3;
	const int16x8_t factor = vdupq_n_s16((int16_t)amount);

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned short *source = (unsigned short *)src;
		unsigned short *destination = (unsigned short *)dest;
		int x = 0;

		for (; x <= (w - 8); x += 8) {
			uint16x8_t source_pixels = vld1q_u16(source + x);
			uint16x8_t destination_pixels = vld1q_u16(
				destination + x);
			uint16x8_t result = simd_blend_rgb565(source_pixels,
				destination_pixels, factor);

			vst1q_u16(destination + x, result);
		}
		for (; x < w; ++x) {
			unsigned int source_color = source[x];
			unsigned int destination_color = destination[x];
			unsigned int source_rb;
			unsigned int source_g;
			unsigned int destination_rb;
			unsigned int destination_g;

			if (source_color == MASK_COLOR_16)
				continue;
			source_rb = source_color & MASK_RB_16;
			source_g = source_color & MASK_G_16;
			destination_rb = destination_color & MASK_RB_16;
			destination_g = destination_color & MASK_G_16;
			source_rb = ((source_rb - destination_rb) * amount) >> 5;
			source_g = ((source_g - destination_g) * amount) >> 5;
			destination[x] = (unsigned short)(
				((destination_rb + source_rb) & MASK_RB_16) |
				((destination_g + source_g) & MASK_G_16));
		}
		src += src_pitch;
		dest += dest_pitch;
	}
}

void fb_hPutBlend4SIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	const uint32x4_t rgb_mask = vdupq_n_u32(FB_GFX_SIMD_RGB_MASK_32);
	const uint32x4_t transparent_color = vdupq_n_u32(MASK_COLOR_32);
	unsigned int factor = (unsigned int)(alpha & 0xFF) + 1u;

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned int *source = (unsigned int *)src;
		unsigned int *destination = (unsigned int *)dest;
		int x = 0;

		for (; x <= (w - 4); x += 4) {
			uint8x16_t source_pixels = vld1q_u8(
				(const uint8_t *)(source + x));
			uint8x16_t destination_pixels = vld1q_u8(
				(const uint8_t *)(destination + x));
			uint8x16_t result = simd_blend_constant(source_pixels,
				destination_pixels, factor);
			uint32x4_t transparent = vceqq_u32(vandq_u32(
				vreinterpretq_u32_u8(source_pixels), rgb_mask),
				transparent_color);

			result = vbslq_u8(vreinterpretq_u8_u32(transparent),
				destination_pixels, result);
			vst1q_u8((uint8_t *)(destination + x), result);
		}
		for (; x < w; ++x) {
			unsigned int source_color = source[x];
			unsigned int destination_color;
			unsigned int source_rb;
			unsigned int source_ga;
			unsigned int destination_rb;
			unsigned int destination_ga;

			if ((source_color & FB_GFX_SIMD_RGB_MASK_32) == MASK_COLOR_32)
				continue;
			destination_color = destination[x];
			source_rb = source_color & MASK_RB_32;
			source_ga = source_color & MASK_GA_32;
			destination_rb = destination_color & MASK_RB_32;
			destination_ga = destination_color & MASK_GA_32;
			source_rb = ((source_rb - destination_rb) * factor) >> 8;
			source_ga = ((source_ga >> 8) - (destination_ga >> 8)) *
				factor;
			destination[x] =
				((destination_rb + source_rb) & MASK_RB_32) |
				((destination_ga + source_ga) & MASK_GA_32);
		}
		src += src_pitch;
		dest += dest_pitch;
	}
}

/* ------------------------------------------------------------------------- */
/* Alpha-mask PUT source                                                     */
/* ------------------------------------------------------------------------- */

void fb_hPutAlphaMaskSIMD(unsigned char *src, unsigned char *dest,
	int w, int h, int src_pitch, int dest_pitch, int alpha,
	BLENDER *blender, void *param)
{
	const uint32x4_t rgb_mask = vdupq_n_u32(FB_GFX_SIMD_RGB_MASK_32);

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned int *destination = (unsigned int *)dest;
		int x = 0;

		for (; x <= (w - 8); x += 8) {
			uint16x8_t source_16 = vmovl_u8(vld1_u8(src + x));
			uint32x4_t alpha_low = vshlq_n_u32(
				vmovl_u16(vget_low_u16(source_16)), 24);
			uint32x4_t alpha_high = vshlq_n_u32(
				vmovl_u16(vget_high_u16(source_16)), 24);
			uint32x4_t destination_low = vld1q_u32(destination + x);
			uint32x4_t destination_high = vld1q_u32(destination + x + 4);

			vst1q_u32(destination + x,
				vorrq_u32(vandq_u32(destination_low, rgb_mask), alpha_low));
			vst1q_u32(destination + x + 4,
				vorrq_u32(vandq_u32(destination_high, rgb_mask), alpha_high));
		}
		for (; x < w; ++x) {
			destination[x] =
				(destination[x] & FB_GFX_SIMD_RGB_MASK_32) |
				((unsigned int)src[x] << 24);
		}
		src += src_pitch;
		dest += dest_pitch;
	}
}

#endif

/* end of gfx_simd_neon.inc */
