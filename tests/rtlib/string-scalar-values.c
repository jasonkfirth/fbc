/*
 * Project: FreeBASIC runtime tests
 * File: string-scalar-values.c
 * Purpose: Verify unsigned parser results and raw wide-character conversions.
 * Responsibilities: Exercise native parser widths, code units and UTF output.
 * Full string descriptor ownership belongs to the public runtime tests.
 */
#include "fb.h"
#include <locale.h>
#include <errno.h>
#include "strings/str_convfrom_uint.c"
#include "strings/str_hskip.c"
#include "strings/wide/strw_convfrom_uint.c"
#include "strings/wide/strw_asc.c"
#include "strings/unicode/ustr_core.c"

static unsigned long checks;

/* These component cases borrow raw buffers and never allocate descriptors.
   COFF linkers still resolve references from unused public helper sections.
   Abort if execution reaches that separate ownership layer. */
FB_RTLIB_CTX fb_ctx;

FBCALL int fb_hStrDelTemp( FBSTRING_ACCESS *str )
{
	(void)str;
	abort();
}

FBCALL int fb_hStrDelTemp_NoLock( FBSTRING_ACCESS *str )
{
	(void)str;
	abort();
}

FBCALL FBSTRING_ACCESS *fb_hStrAllocTemp_NoLock( FBSTRING_ACCESS *str, ssize_t size )
{
	(void)str;
	(void)size;
	abort();
}

FBCALL void fb_StrDelete( FBSTRING_ACCESS *str )
{
	(void)str;
	abort();
}

FBCALL int fb_ErrorSetNum( int errnum )
{
	(void)errnum;
	abort();
}

static void check( int condition )
{
	checks++;
	if( !condition ) {
		fprintf( stderr, "STRING_SCALAR_VALUES_FAIL check=%lu\n", checks );
		exit(1);
	}
}

static void check_parser( const char *text, const char *digits, int radix )
{
	char bytes[128];
	FB_WCHAR wide[128];
	size_t length = strlen(text);
	unsigned int expected;
	int parse_error;
	check( length < sizeof(bytes) );
	memcpy( bytes, text, length + 1 );
	for( size_t i = 0; i <= length; i++ )
		wide[i] = (FB_WCHAR)(unsigned char)text[i];
	errno = 0;
	expected = (unsigned int)strtoul( digits, NULL, radix );
	parse_error = errno;
	errno = 0;
	check( fb_hStr2UInt( bytes, (ssize_t)length ) == expected );
	check( errno == parse_error );
	errno = 0;
	check( fb_WstrToUInt( wide, (ssize_t)length ) == expected );
	check( errno == parse_error );
	check( fb_WstrValUInt( wide ) == expected );
}

static void check_wide( uint32_t first, uint32_t second )
{
	FB_WCHAR text[3] = { (FB_WCHAR)first, (FB_WCHAR)second, 0 };
	ssize_t offset = 0;
	unsigned int expected = first;
	char encoded[8];
	ssize_t size;
	check( fb_WstrAsc( text, 1 ) == first );
	check( fb_WstrAsc( text, 0 ) == 0 );
	check( fb_WstrAsc( text, -1 ) == 0 );
	check( fb_WstrAsc( text, 3 ) == 0 );
	if( sizeof(FB_WCHAR) == 2 && first >= 0xd800u && first <= 0xdbffu ) {
		if( second >= 0xdc00u && second <= 0xdfffu ) {
			expected = 0x10000u + ((first - 0xd800u) << 10) + second - 0xdc00u;
			offset = 2;
		} else {
			expected = 0xfffdu;
			offset = 1;
		}
	} else {
		offset = 1;
	}
	{
		ssize_t actual_offset = 0;
		check( hWideDecode( text, &actual_offset ) == expected );
		check( actual_offset == offset );
	}
	memset( encoded, 0x5a, sizeof(encoded) );
	size = fb_hUtf8Encode( encoded, expected );
	check( size >= 1 && size <= 4 );
	check( encoded[size] == 0x5a );
	if( expected > 0x10ffffu || (expected >= 0xd800u && expected <= 0xdfffu) ) {
		check( size == 3 );
		check( memcmp( encoded, "\xef\xbf\xbd", 3 ) == 0 );
	}
}

int main( void )
{
	check( sizeof(unsigned int) == 4 );
	check( sizeof(FB_WCHAR) == 2 || sizeof(FB_WCHAR) == 4 );
	check( setlocale( LC_ALL, "C" ) != NULL );
	check_parser( "", "", 10 );
	check_parser( "   ", "", 10 );
	check_parser( "invalid", "invalid", 10 );
	check_parser( "0", "0", 10 );
	check_parser( "42tail", "42tail", 10 );
	check_parser( " +42", "+42", 10 );
	check_parser( "-1", "-1", 10 );
	check_parser( "-4294967296", "-4294967296", 10 );
	check_parser( "4294967295", "4294967295", 10 );
	check_parser( "4294967296", "4294967296", 10 );
	check_parser( "4294967297", "4294967297", 10 );
	check_parser( "18446744073709551615", "18446744073709551615", 10 );
	check_parser( "18446744073709551616", "18446744073709551616", 10 );
	check_parser( "&Hffffffff", "ffffffff", 16 );
	check_parser( "  &H100000001", "100000001", 16 );
	check_parser( "&hffffffffffffffff", "ffffffffffffffff", 16 );
	check_parser( "&O37777777777", "37777777777", 8 );
	check_parser( "&o40000000001", "40000000001", 8 );
	check_parser( "&377", "377", 8 );
	check_parser( "&B11111111", "11111111", 2 );
	check_parser( "&b100000000000000000000000000000001", "100000000000000000000000000000001", 2 );
	check_parser( "&H", "", 16 );
	check_parser( "&x", "x", 8 );
	check_parser( "&", "&", 10 );
	check( fb_WstrToUInt( NULL, 10 ) == 0 );
	check( fb_WstrValUInt( NULL ) == 0 );
	check( fb_WstrAsc( NULL, 1 ) == 0 );
	for( uint32_t point = 1; point <= (sizeof(FB_WCHAR) == 2 ? 0xffffu : 0x10ffffu); point++ )
		check_wide( point, 0 );
	if( sizeof(FB_WCHAR) == 2 ) {
		for( uint32_t high = 0xd800u; high <= 0xdbffu; high++ ) {
			for( uint32_t low = 0xdc00u; low <= 0xdfffu; low++ )
				check_wide( high, low );
			check_wide( high, 0x41 );
		}
	} else {
		check_wide( 0x110000u, 0 );
		check_wide( 0x7fffffffu, 0 );
		check_wide( 0x80000000u, 0 );
		check_wide( 0xffffffffu, 0 );
	}
	fprintf( stderr, "STRING_SCALAR_VALUES_PASS checks=%lu wchar=%zu ulong=%zu\n",
		checks, sizeof(FB_WCHAR), sizeof(unsigned long) );
	return 0;
}

/* end of string-scalar-values.c */
