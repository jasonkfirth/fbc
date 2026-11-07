/*
 * Project: FreeBASIC runtime tests
 * File: utf-word-values.c
 * Purpose: Verify the shared little-endian word helpers at real byte boundaries.
 * Responsibilities: Check all 16-bit words, 32-bit patterns and adjacent bytes.
 * This file intentionally does NOT decode text or exercise image libraries.
 */
#include "fb.h"

static int check16( UTF_16 value )
{
	unsigned char buffer[16] __attribute__((aligned(8)));
	/* Base-256 arithmetic provides an independent byte-order reference. */
	const unsigned char expected[2] = {
		(unsigned char)(value % 256), (unsigned char)(value / 256)
	};

	/* Eight-byte alignment makes offsets 1 and 7 unaligned, while 4 is aligned. */
#define CHECK16_AT(offset) do { \
	memset( buffer, 0xa5, sizeof( buffer ) ); \
	fb_UTF16ToLE( &buffer[offset], value ); \
	if( memcmp( &buffer[offset], expected, sizeof( expected ) ) != 0 || \
	    buffer[(offset) - 1] != 0xa5 || buffer[(offset) + 2] != 0xa5 || \
	    fb_UTF16FromLE( &buffer[offset] ) != value ) return 1; \
} while( 0 )
	CHECK16_AT( 1 );
	CHECK16_AT( 4 );
	CHECK16_AT( 7 );
#undef CHECK16_AT
	return 0;
}

static int check32( UTF_32 value )
{
	unsigned char buffer[16] __attribute__((aligned(8)));
	const unsigned char expected[4] = {
		(unsigned char)(value % UINT32_C(256)),
		(unsigned char)((value / UINT32_C(256)) % UINT32_C(256)),
		(unsigned char)((value / UINT32_C(65536)) % UINT32_C(256)),
		(unsigned char)(value / UINT32_C(16777216))
	};

#define CHECK32_AT(offset) do { \
	memset( buffer, 0xa5, sizeof( buffer ) ); \
	fb_UTF32ToLE( &buffer[offset], value ); \
	if( memcmp( &buffer[offset], expected, sizeof( expected ) ) != 0 || \
	    buffer[(offset) - 1] != 0xa5 || buffer[(offset) + 4] != 0xa5 || \
	    fb_UTF32FromLE( &buffer[offset] ) != value ) return 1; \
} while( 0 )
	CHECK32_AT( 1 );
	CHECK32_AT( 4 );
	CHECK32_AT( 7 );
#undef CHECK32_AT
	return 0;
}

int main( void )
{
	UTF_32 value;
	unsigned int bit;

	for( value = 0; value <= UINT32_C(0xffff); ++value ) {
		if( check16( (UTF_16)value ) ) return 1;
	}
	if( check32( 0 ) || check32( UINT32_MAX ) ||
	    check32( UINT32_C(0x01234567) ) || check32( UINT32_C(0x89abcdef) ) ) return 2;
	for( bit = 0; bit < 32; ++bit ) {
		value = UINT32_C(1) << bit;
		if( check32( value ) || check32( ~value ) ) return 3;
	}
	puts( "UTF_WORD_VALUES_PASS u16=65536 u32=68 offsets=1,4,7" );
	return 0;
}

/* end of utf-word-values.c */
