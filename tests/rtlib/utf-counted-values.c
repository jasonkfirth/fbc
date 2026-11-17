/*
 * Project: FreeBASIC runtime tests
 * File: utf-counted-values.c
 * Purpose: Verify counted wide input in the native UTF encoders.
 * Responsibilities: Check code-unit spans, scalar values and output boundaries.
 * Locale-dependent console rendering belongs to the Unix console regression.
 */
#ifndef FB_UTF_FROM_WCHAR_SOURCE
#define FB_UTF_FROM_WCHAR_SOURCE "strings/encoding/utf_convfrom_wchar.c"
#endif
#include FB_UTF_FROM_WCHAR_SOURCE
#include "strings/encoding/utf_core.c"

static unsigned long checks;

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "UTF_COUNTED_VALUES_FAIL check=%lu\n", checks );
		exit(1);
	}
}

static size_t reference_utf8( uint32_t point, unsigned char *output )
{
	if( point < UINT32_C(0x80) ) {
		output[0] = (unsigned char)point;
		return 1;
	}
	if( point < UINT32_C(0x800) ) {
		output[0] = (unsigned char)(UINT32_C(0xc0) | (point >> 6));
		output[1] = (unsigned char)(UINT32_C(0x80) | (point & 63u));
		return 2;
	}
	if( point < UINT32_C(0x10000) ) {
		output[0] = (unsigned char)(UINT32_C(0xe0) | (point >> 12));
		output[1] = (unsigned char)(UINT32_C(0x80) | ((point >> 6) & 63u));
		output[2] = (unsigned char)(UINT32_C(0x80) | (point & 63u));
		return 3;
	}
	output[0] = (unsigned char)(UINT32_C(0xf0) | (point >> 18));
	output[1] = (unsigned char)(UINT32_C(0x80) | ((point >> 12) & 63u));
	output[2] = (unsigned char)(UINT32_C(0x80) | ((point >> 6) & 63u));
	output[3] = (unsigned char)(UINT32_C(0x80) | (point & 63u));
	return 4;
}

static ssize_t source_units( uint32_t point, FB_WCHAR *source, int utf16 )
{
	if( utf16 && point >= UINT32_C(0x10000) ) {
		uint32_t offset = point - UINT32_C(0x10000);
		source[0] = (FB_WCHAR)(UINT32_C(0xd800) + (offset >> 10));
		source[1] = (FB_WCHAR)(UINT32_C(0xdc00) + (offset & 1023u));
		return 2;
	}
	source[0] = (FB_WCHAR)point;
	/* A second stored unit must stay outside a one-unit conversion. */
	source[1] = (FB_WCHAR)UINT32_C(0xdc00);
	return 1;
}

static void check_scalar( uint32_t point )
{
	FB_WCHAR source[2];
	unsigned char output[16], expected8[4];
	UTF_16 expected16[2];
	UTF_32 expected32 = point;
	ssize_t bytes, units;
	size_t expected_bytes = reference_utf8(point, expected8);
	ssize_t expected16_bytes = 2;

	units = source_units(point, source, sizeof(FB_WCHAR) == 2);
	memset(output, 0xa5, sizeof(output));
	check(fb_WCharToUTF(FB_FILE_ENCOD_UTF8, source, units, (char *)output + 1, &bytes) == (char *)output + 1);
	check(bytes == (ssize_t)expected_bytes && memcmp(output + 1, expected8, expected_bytes) == 0);
	check(output[0] == 0xa5 && output[expected_bytes + 1] == 0xa5);

	if( point >= UINT32_C(0x10000) ) {
		uint32_t offset = point - UINT32_C(0x10000);
		expected16[0] = (UTF_16)(UINT32_C(0xd800) + (offset >> 10));
		expected16[1] = (UTF_16)(UINT32_C(0xdc00) + (offset & 1023u));
		expected16_bytes = 4;
	} else {
		expected16[0] = (UTF_16)point;
	}
	memset(output, 0xa5, sizeof(output));
	check(fb_WCharToUTF(FB_FILE_ENCOD_UTF16, source, units, (char *)output + 1, &bytes) == (char *)output + 1);
	check(bytes == expected16_bytes && memcmp(output + 1, expected16, (size_t)bytes) == 0);
	check(output[0] == 0xa5 && output[(size_t)bytes + 1] == 0xa5);
	memset(output, 0xa5, sizeof(output));
	check(fb_WCharToUTF(FB_FILE_ENCOD_UTF32, source, units, (char *)output + 1, &bytes) == (char *)output + 1);
	check(bytes == 4 && memcmp(output + 1, &expected32, sizeof(expected32)) == 0);
	check(output[0] == 0xa5 && output[5] == 0xa5);

	/* Exercise the actual UTF16 helpers on every host. Native public calls
	   above select the platform's own FB_WCHAR representation. */
	units = source_units(point, source, TRUE);
	memset(output, 0xa5, sizeof(output));
	hUTF16ToUTF8(source, units, output + 1, &bytes);
	check(bytes == (ssize_t)expected_bytes && memcmp(output + 1, expected8, expected_bytes) == 0);
	check(output[0] == 0xa5 && output[expected_bytes + 1] == 0xa5);
	memset(output, 0xa5, sizeof(output));
	bytes = units * 4;
	hUTF16ToUTF32(source, units, (char *)output + 1, &bytes);
	check(bytes == 4 && memcmp(output + 1, &expected32, sizeof(expected32)) == 0);
	check(output[0] == 0xa5 && output[5] == 0xa5);
}

static void check_spans( void )
{
	FB_WCHAR source[3] = { (FB_WCHAR)0xd800u, (FB_WCHAR)'A', 0 };
	const unsigned char expected8[4] = { 0xed, 0xa0, 0x80, 'A' };
	const UTF_32 expected32[2] = { UINT32_C(0xd800), UINT32_C(0x41) };
	unsigned char output[16];
	ssize_t bytes;

	memset(output, 0xa5, sizeof(output));
	hUTF16ToUTF8(source, 2, output + 1, &bytes);
	check(bytes == 4 && memcmp(output + 1, expected8, sizeof(expected8)) == 0);
	check(output[0] == 0xa5 && output[5] == 0xa5);
	bytes = 8;
	hUTF16ToUTF32(source, 2, (char *)output + 1, &bytes);
	check(bytes == 8 && memcmp(output + 1, expected32, sizeof(expected32)) == 0);
	check(output[0] == 0xa5 && output[9] == 0xa5);
	bytes = 123;
	check(fb_WCharToUTF(FB_FILE_ENCOD_UTF8, NULL, -1, (char *)output, &bytes) == NULL && bytes == 0);
	check(fb_WCharToUTF(FB_FILE_ENCOD_UTF8, NULL, 1, (char *)output, &bytes) == NULL && bytes == 0);
	check(fb_WCharToUTF(FB_FILE_ENCOD_UTF8, source, (ssize_t)(FB_STRSIZEMSK / 4 + 1), (char *)output, &bytes) == NULL && bytes == 0);
	check(fb_WCharToUTF(FB_FILE_ENCOD_UTF8, NULL, 0, (char *)output, &bytes) == (char *)output && bytes == 0);
}

int main( void )
{
	uint32_t point;
	for( point = 0; point <= UINT32_C(0x10ffff); point++ )
		check_scalar(point);
	check_spans();
	printf("UTF_COUNTED_VALUES_PASS checks=%lu wchar=%zu\n", checks, sizeof(FB_WCHAR));
	return 0;
}

/* end of utf-counted-values.c */
