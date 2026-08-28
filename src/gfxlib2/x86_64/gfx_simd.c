/*
    Project: FreeBASIC gfxlib2 x86_64 SIMD
    --------------------------------------

    File: gfx_simd.c

    Purpose:

        Accelerate common software-framebuffer drawing operations with the
        SSE2 instruction set guaranteed by the x86_64 ABI.

    Responsibilities:

        - fill 16-bit and 32-bit pixel spans
        - blend alpha-bearing 32-bit pixels with exact integer arithmetic
        - accelerate logical, transparent, additive, and blended PUT modes
        - handle arbitrary row alignment and scalar tails safely

    This file intentionally does NOT contain:

        - CPU feature probing for optional post-SSE2 extensions
        - 32-bit x86 MMX compatibility code
        - framebuffer locking or dirty-line management
*/

#include "../gfx_simd.h"

#include <emmintrin.h>

enum FB_GFX_SIMD_LOGIC {
	FB_GFX_SIMD_AND,
	FB_GFX_SIMD_OR,
	FB_GFX_SIMD_XOR,
	FB_GFX_SIMD_PRESET
};

/* All 32-bit gfxlib2 drawing colours store RGB in the low 24 bits. */
#define FB_GFX_SIMD_RGB_MASK_32 0x00FFFFFFu

/* ------------------------------------------------------------------------- */
/* Shared SSE2 helpers                                                       */
/* ------------------------------------------------------------------------- */

static __m128i simd_select(__m128i mask, __m128i when_true,
	__m128i when_false)
{
	return _mm_or_si128(_mm_and_si128(mask, when_true),
		_mm_andnot_si128(mask, when_false));
}

static __m128i simd_blend_factors(__m128i source, __m128i destination,
	__m128i alpha_low, __m128i alpha_high)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i full = _mm_set1_epi16(256);
	__m128i source_low;
	__m128i source_high;
	__m128i destination_low;
	__m128i destination_high;
	__m128i inverse_low;
	__m128i inverse_high;
	__m128i result_low;
	__m128i result_high;

	source_low = _mm_unpacklo_epi8(source, zero);
	source_high = _mm_unpackhi_epi8(source, zero);
	destination_low = _mm_unpacklo_epi8(destination, zero);
	destination_high = _mm_unpackhi_epi8(destination, zero);
	inverse_low = _mm_sub_epi16(full, alpha_low);
	inverse_high = _mm_sub_epi16(full, alpha_high);
	result_low = _mm_add_epi16(_mm_mullo_epi16(source_low, alpha_low),
		_mm_mullo_epi16(destination_low, inverse_low));
	result_high = _mm_add_epi16(
		_mm_mullo_epi16(source_high, alpha_high),
		_mm_mullo_epi16(destination_high, inverse_high));
	result_low = _mm_srli_epi16(result_low, 8);
	result_high = _mm_srli_epi16(result_high, 8);
	return _mm_packus_epi16(result_low, result_high);
}

static __m128i simd_blend_source_alpha(__m128i source,
	__m128i destination, int add_one, int preserve_source_alpha)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i one = _mm_set1_epi16(1);
	const __m128i alpha_mask = _mm_set1_epi32((int)MASK_A_32);
	__m128i source_low;
	__m128i source_high;
	__m128i alpha_low;
	__m128i alpha_high;
	__m128i result;

	source_low = _mm_unpacklo_epi8(source, zero);
	source_high = _mm_unpackhi_epi8(source, zero);
	alpha_low = _mm_shufflelo_epi16(source_low,
		_MM_SHUFFLE(3, 3, 3, 3));
	alpha_low = _mm_shufflehi_epi16(alpha_low,
		_MM_SHUFFLE(3, 3, 3, 3));
	alpha_high = _mm_shufflelo_epi16(source_high,
		_MM_SHUFFLE(3, 3, 3, 3));
	alpha_high = _mm_shufflehi_epi16(alpha_high,
		_MM_SHUFFLE(3, 3, 3, 3));
	if (add_one) {
		alpha_low = _mm_add_epi16(alpha_low, one);
		alpha_high = _mm_add_epi16(alpha_high, one);
	}
	result = simd_blend_factors(source, destination, alpha_low,
		alpha_high);
	if (preserve_source_alpha)
		result = simd_select(alpha_mask, source, result);
	return result;
}

static __m128i simd_blend_constant(__m128i source,
	__m128i destination, unsigned int alpha)
{
	const __m128i factor = _mm_set1_epi16((short)alpha);

	return simd_blend_factors(source, destination, factor, factor);
}

static __m128i simd_blend_rgb565(__m128i source, __m128i destination,
	__m128i factor)
{
	const __m128i mask_5 = _mm_set1_epi16(0x1F);
	const __m128i mask_6 = _mm_set1_epi16(0x3F);
	const __m128i transparent_color = _mm_set1_epi16((short)MASK_COLOR_16);
	__m128i source_r = _mm_and_si128(_mm_srli_epi16(source, 11), mask_5);
	__m128i source_g = _mm_and_si128(_mm_srli_epi16(source, 5), mask_6);
	__m128i source_b = _mm_and_si128(source, mask_5);
	__m128i destination_r = _mm_and_si128(
		_mm_srli_epi16(destination, 11), mask_5);
	__m128i destination_g = _mm_and_si128(
		_mm_srli_epi16(destination, 5), mask_6);
	__m128i destination_b = _mm_and_si128(destination, mask_5);
	__m128i result_r;
	__m128i result_g;
	__m128i result_b;
	__m128i result;
	__m128i transparent;

	result_r = _mm_srai_epi16(_mm_mullo_epi16(
		_mm_sub_epi16(source_r, destination_r), factor), 5);
	result_g = _mm_srai_epi16(_mm_mullo_epi16(
		_mm_sub_epi16(source_g, destination_g), factor), 5);
	result_b = _mm_srai_epi16(_mm_mullo_epi16(
		_mm_sub_epi16(source_b, destination_b), factor), 5);
	result_r = _mm_add_epi16(destination_r, result_r);
	result_g = _mm_add_epi16(destination_g, result_g);
	result_b = _mm_add_epi16(destination_b, result_b);
	result = _mm_or_si128(_mm_slli_epi16(result_r, 11),
		_mm_or_si128(_mm_slli_epi16(result_g, 5), result_b));
	transparent = _mm_cmpeq_epi16(source, transparent_color);
	return simd_select(transparent, destination, result);
}

/* ------------------------------------------------------------------------- */
/* Feature contract and pixel fills                                          */
/* ------------------------------------------------------------------------- */

int fb_hSimdAvailable(void)
{
	/* SSE2 is part of the x86_64 ABI and therefore needs no runtime probe. */
	return TRUE;
}

void *fb_hPixelSet2SIMD(void *dest, int color, size_t size)
{
	unsigned short *cursor = (unsigned short *)dest;
	const __m128i fill = _mm_set1_epi16((short)color);

	while (size >= 8u) {
		_mm_storeu_si128((__m128i *)cursor, fill);
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
	const __m128i fill = _mm_set1_epi32(color);

	while (size >= 4u) {
		_mm_storeu_si128((__m128i *)cursor, fill);
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
	const __m128i source = _mm_set1_epi32(color);
	unsigned int source_color = (unsigned int)color;
	unsigned int source_rb = source_color & MASK_RB_32;
	unsigned int source_g = source_color & MASK_G_32;
	unsigned int source_a = source_color & MASK_A_32;
	unsigned int alpha = source_color >> 24;

	while (size >= 4u) {
		__m128i destination = _mm_loadu_si128((const __m128i *)cursor);
		__m128i result = simd_blend_source_alpha(source, destination,
			FALSE, TRUE);

		_mm_storeu_si128((__m128i *)cursor, result);
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
	const __m128i all_bits = _mm_set1_epi32(-1);
	size_t offset = 0u;

	while ((bytes - offset) >= 16u) {
		__m128i source_pixels = _mm_loadu_si128(
			(const __m128i *)(source + offset));
		__m128i destination_pixels = _mm_loadu_si128(
			(const __m128i *)(destination + offset));
		__m128i result;

		switch (operation) {
		case FB_GFX_SIMD_AND:
			result = _mm_and_si128(destination_pixels, source_pixels);
			break;
		case FB_GFX_SIMD_OR:
			result = _mm_or_si128(destination_pixels, source_pixels);
			break;
		case FB_GFX_SIMD_XOR:
			result = _mm_xor_si128(destination_pixels, source_pixels);
			break;
		default:
			result = _mm_xor_si128(source_pixels, all_bits);
			break;
		}
		_mm_storeu_si128((__m128i *)(destination + offset), result);
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
	const __m128i zero = _mm_setzero_si128();

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		int x = 0;

		for (; x <= (w - 16); x += 16) {
			__m128i source = _mm_loadu_si128((const __m128i *)(src + x));
			__m128i destination = _mm_loadu_si128(
				(const __m128i *)(dest + x));
			__m128i transparent = _mm_cmpeq_epi8(source, zero);

			_mm_storeu_si128((__m128i *)(dest + x),
				simd_select(transparent, destination, source));
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
	const __m128i transparent_color = _mm_set1_epi16((short)MASK_COLOR_16);

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned short *source = (unsigned short *)src;
		unsigned short *destination = (unsigned short *)dest;
		int x = 0;

		for (; x <= (w - 8); x += 8) {
			__m128i source_pixels = _mm_loadu_si128(
				(const __m128i *)(source + x));
			__m128i destination_pixels = _mm_loadu_si128(
				(const __m128i *)(destination + x));
			__m128i transparent = _mm_cmpeq_epi16(source_pixels,
				transparent_color);

			_mm_storeu_si128((__m128i *)(destination + x),
				simd_select(transparent, destination_pixels,
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
	const __m128i rgb_mask = _mm_set1_epi32((int)FB_GFX_SIMD_RGB_MASK_32);
	const __m128i transparent_color = _mm_set1_epi32(MASK_COLOR_32);

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned int *source = (unsigned int *)src;
		unsigned int *destination = (unsigned int *)dest;
		int x = 0;

		for (; x <= (w - 4); x += 4) {
			__m128i source_pixels = _mm_loadu_si128(
				(const __m128i *)(source + x));
			__m128i destination_pixels = _mm_loadu_si128(
				(const __m128i *)(destination + x));
			__m128i colors = _mm_and_si128(source_pixels, rgb_mask);
			__m128i transparent = _mm_cmpeq_epi32(colors,
				transparent_color);

			_mm_storeu_si128((__m128i *)(destination + x),
				simd_select(transparent, destination_pixels, colors));
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
			__m128i source_pixels = _mm_loadu_si128(
				(const __m128i *)(source + x));
			__m128i destination_pixels = _mm_loadu_si128(
				(const __m128i *)(destination + x));
			__m128i result = simd_blend_source_alpha(source_pixels,
				destination_pixels, TRUE, FALSE);

			_mm_storeu_si128((__m128i *)(destination + x), result);
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
	const __m128i zero = _mm_setzero_si128();
	const __m128i factor = _mm_set1_epi16((short)(alpha & 0xFF));
	const __m128i rgb_mask = _mm_set1_epi32((int)FB_GFX_SIMD_RGB_MASK_32);
	const __m128i transparent_color = _mm_set1_epi32(MASK_COLOR_32);
	unsigned int amount = (unsigned int)alpha & 0xFFu;

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned int *source = (unsigned int *)src;
		unsigned int *destination = (unsigned int *)dest;
		int x = 0;

		for (; x <= (w - 4); x += 4) {
			__m128i source_pixels = _mm_loadu_si128(
				(const __m128i *)(source + x));
			__m128i destination_pixels = _mm_loadu_si128(
				(const __m128i *)(destination + x));
			__m128i source_low = _mm_unpacklo_epi8(source_pixels, zero);
			__m128i source_high = _mm_unpackhi_epi8(source_pixels, zero);
			__m128i scaled;
			__m128i result;
			__m128i transparent;

			source_low = _mm_srli_epi16(
				_mm_mullo_epi16(source_low, factor), 8);
			source_high = _mm_srli_epi16(
				_mm_mullo_epi16(source_high, factor), 8);
			scaled = _mm_packus_epi16(source_low, source_high);
			result = _mm_adds_epu8(scaled, destination_pixels);
			transparent = _mm_cmpeq_epi32(
				_mm_and_si128(source_pixels, rgb_mask),
				transparent_color);
			result = simd_select(transparent, destination_pixels, result);
			_mm_storeu_si128((__m128i *)(destination + x), result);
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
	const __m128i factor = _mm_set1_epi16((short)amount);

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned short *source = (unsigned short *)src;
		unsigned short *destination = (unsigned short *)dest;
		int x = 0;

		for (; x <= (w - 8); x += 8) {
			__m128i source_pixels = _mm_loadu_si128(
				(const __m128i *)(source + x));
			__m128i destination_pixels = _mm_loadu_si128(
				(const __m128i *)(destination + x));
			__m128i result = simd_blend_rgb565(source_pixels,
				destination_pixels, factor);

			_mm_storeu_si128((__m128i *)(destination + x), result);
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
	const __m128i rgb_mask = _mm_set1_epi32((int)FB_GFX_SIMD_RGB_MASK_32);
	const __m128i transparent_color = _mm_set1_epi32(MASK_COLOR_32);
	unsigned int factor = (unsigned int)(alpha & 0xFF) + 1u;

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned int *source = (unsigned int *)src;
		unsigned int *destination = (unsigned int *)dest;
		int x = 0;

		for (; x <= (w - 4); x += 4) {
			__m128i source_pixels = _mm_loadu_si128(
				(const __m128i *)(source + x));
			__m128i destination_pixels = _mm_loadu_si128(
				(const __m128i *)(destination + x));
			__m128i result = simd_blend_constant(source_pixels,
				destination_pixels, factor);
			__m128i transparent = _mm_cmpeq_epi32(
				_mm_and_si128(source_pixels, rgb_mask),
				transparent_color);

			result = simd_select(transparent, destination_pixels, result);
			_mm_storeu_si128((__m128i *)(destination + x), result);
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
	const __m128i rgb_mask = _mm_set1_epi32((int)FB_GFX_SIMD_RGB_MASK_32);

	if ((w <= 0) || (h <= 0))
		return;
	while (h-- != 0) {
		unsigned int *destination = (unsigned int *)dest;
		int x = 0;

		for (; x <= (w - 4); x += 4) {
			unsigned int packed_alpha;
			__m128i source_alpha;
			__m128i destination_pixels = _mm_loadu_si128(
				(const __m128i *)(destination + x));

			memcpy(&packed_alpha, src + x, sizeof(packed_alpha));
			source_alpha = _mm_cvtsi32_si128((int)packed_alpha);
			source_alpha = _mm_unpacklo_epi8(source_alpha,
				_mm_setzero_si128());
			source_alpha = _mm_unpacklo_epi16(source_alpha,
				_mm_setzero_si128());
			source_alpha = _mm_slli_epi32(source_alpha, 24);
			_mm_storeu_si128((__m128i *)(destination + x),
				_mm_or_si128(_mm_and_si128(destination_pixels, rgb_mask),
					source_alpha));
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

/* end of gfx_simd.c */
