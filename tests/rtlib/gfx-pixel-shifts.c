/*
 * Project: FreeBASIC graphics tests
 * File: gfx-pixel-shifts.c
 * Purpose: Check opaque BMP conversion and paired RGB565 blending.
 * Responsibilities: Compare production pixel kernels with scalar references.
 * This file does not create a display or exercise graphics drivers.
 *
 * Link with gfxlib2 and rtlib. The included production files expose their
 * static kernels to this test; the archive supplies their unused API helpers.
 */
#include "../../src/gfxlib2/image/gfx_bload.c"
#include "../../src/gfxlib2/pixels/gfx_put_blend.c"

static int checks;

static void check( int condition )
{
	++checks;
	if( !condition ) {
		fprintf( stderr, "GFX_PIXEL_SHIFTS_FAIL check=%d\n", checks );
		exit( 1 );
	}
}

static uint16_t blend565( uint16_t src, uint16_t dst, unsigned int alpha )
{
	uint32_t rb, green;
	if( src == 0xf81fu )
		return dst;
	alpha = (alpha + 7u) / 8u;
	rb = ((uint32_t)dst & 0xf81fu) +
		(((((uint32_t)src & 0xf81fu) - ((uint32_t)dst & 0xf81fu)) * alpha) >> 5);
	green = ((uint32_t)dst & 0x07e0u) +
		(((((uint32_t)src & 0x07e0u) - ((uint32_t)dst & 0x07e0u)) * alpha) >> 5);
	return (uint16_t)((rb & 0xf81fu) | (green & 0x07e0u));
}

int main( void )
{
	const uint32_t masks16[3] = {31, 63, 31}, masks24[3] = {255, 255, 255};
	const int shifts16[3] = {11, 5, 0}, bits16[3] = {5, 6, 5};
	const int shifts24[3] = {16, 8, 0}, bits24[3] = {8, 8, 8};
	uint32_t pixel;
	unsigned int value, alpha;
	unsigned char bytes[3];
	uint16_t source[4], target[4], expected[4];
	int i;

	for( value = 0; value <= 65535; ++value ) {
		uint32_t r = value / 2048u, g = (value / 32u) % 64u, b = value % 32u;
		bytes[0] = (unsigned char)value;
		bytes[1] = (unsigned char)(value / 256u);
		convert_bf_16to32( bytes, (unsigned char *)&pixel, 1, masks16, shifts16, bits16 );
		check( pixel == (0xff000000u | ((r * 8u + r / 4u) << 16) |
			((g * 4u + g / 16u) << 8) | (b * 8u + b / 4u)) );
		bytes[0] = (unsigned char)value;
		bytes[1] = (unsigned char)(value / 256u);
		bytes[2] = (unsigned char)(value ^ 0xa5u);
		convert_bf_24to32( bytes, (unsigned char *)&pixel, 1, masks24, shifts24, bits24 );
		check( pixel == 0xff000000u + (uint32_t)bytes[2] * 65536u +
			(uint32_t)bytes[1] * 256u + bytes[0] );
	}

	/* Include color-key pixels in either half of the packed two-pixel path. */
	for( alpha = 0; alpha <= 255; ++alpha ) {
		for( value = 0; value < 256; ++value ) {
			for( i = 0; i < 4; ++i ) {
				source[i] = (uint16_t)((value * 257u + (unsigned int)i * 12345u) % 65536u);
				target[i] = (uint16_t)(65535u - value * 193u - (unsigned int)i * 511u);
			}
			if( value % 3u == 0 ) source[0] = 0xf81fu;
			if( value % 5u == 0 ) source[1] = 0xf81fu;
			for( i = 0; i < 4; ++i ) expected[i] = blend565( source[i], target[i], alpha );
			fb_hPutBlend2C( (unsigned char *)source, (unsigned char *)target,
				4, 1, 8, 8, (int)alpha, NULL, NULL );
			check( memcmp( target, expected, sizeof( target ) ) == 0 );
		}
	}
	printf( "GFX_PIXEL_SHIFTS_PASS checks=%d\n", checks );
	return 0;
}

/* end of gfx-pixel-shifts.c */
