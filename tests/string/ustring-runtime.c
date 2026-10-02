/*
    FreeBASIC tests
    File: ustring-runtime.c
    Purpose: Exercise UTF-8 runtime boundaries under address/UB sanitizers.
    Responsibilities: Check every Unicode scalar, malformed bytes, casing,
        allocation limits, aliasing, and repeated temporary ownership.
    This file intentionally does NOT contain compiler integration tests.
*/

#include "fb.h"
#include <assert.h>

typedef struct {
	unsigned int scalar, count, mapped[3];
} UTF8_CASE_MAP;
typedef struct {
	unsigned int first, last;
} UTF8_CASE_RANGE;
#include "ustr_case_data.h"

static void checkBytes( FBSTRING *str, const char *expected, ssize_t length )
{
	assert( str != NULL );
	assert( FB_STRSIZE(str) == length );
	if( length ) {
		assert( memcmp( str->data, expected, length ) == 0 );
		assert( str->data[length] == 0 );
	}
	fb_hStrDelTemp( str );
}

static void checkScalars( void )
{
	unsigned int scalar;
	ssize_t length, offset;
	char encoded[4];
	for( scalar = 0; scalar <= 0x10FFFF; ++scalar ) {
		if( (scalar >= 0xD800) && (scalar <= 0xDFFF) ) continue;
		length = fb_hUtf8Encode( encoded, scalar );
		offset = 0;
		assert( fb_hUtf8Decode( encoded, length, &offset ) == scalar );
		assert( offset == length );
		assert( fb_hUtf8Count( encoded, length ) == 1 );
		assert( length == (scalar < 0x80 ? 1 : scalar < 0x800 ? 2 : scalar < 0x10000 ? 3 : 4) );
	}
}

static void checkMalformed( void )
{
	static const char replacement[] = "\xEF\xBF\xBD";
	checkBytes( fb_UStrFromBytes( "\xC0\x80", 3 ), "\xEF\xBF\xBD\xEF\xBF\xBD", 6 );
	checkBytes( fb_UStrFromBytes( "\xE2\x82" "A", 4 ), "\xEF\xBF\xBD" "A", 4 );
	checkBytes( fb_UStrFromBytes( "\xF0\x9F\x98", 4 ), replacement, 3 );
	checkBytes( fb_UStrFromBytes( "\xED\xA0\x80", 4 ), "\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD", 9 );
	checkBytes( fb_UStrFromBytes( "\xF4\x90\x80\x80", 5 ), "\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD", 12 );
	checkBytes( fb_UStrFromBytes( "A\0B", FB_STRISFIXED | 3 ), "A\0B", 3 );
	checkBytes( fb_UStrFromBytes( NULL, 0 ), "", 0 );
}

static void checkMaps( const UTF8_CASE_MAP *map, size_t count, int lower )
{
	size_t i;
	unsigned int j;
	ssize_t input_length, output_length;
	char input[4], expected[12];
	FBSTRING descriptor;
	for( i = 0; i < count; ++i ) {
		input_length = fb_hUtf8Encode( input, map[i].scalar );
		output_length = 0;
		for( j = 0; j < map[i].count; ++j )
			output_length += fb_hUtf8Encode( expected + output_length, map[i].mapped[j] );
		descriptor.data = input;
		descriptor.len = descriptor.size = input_length;
		checkBytes( lower ? fb_UStrLcase( &descriptor, 0 ) : fb_UStrUcase( &descriptor, 0 ), expected, output_length );
	}
}

static void checkOwnership( void )
{
	FBSTRING dst = { NULL, 0, 0 }, source;
	char raw[32];
	unsigned int seed = 123456789u;
	ssize_t i, j, length;
	FBSTRING *normalized, *again;
	for( i = 0; i < 10000; ++i ) {
		for( j = 0; j < (ssize_t)sizeof(raw); ++j ) {
			/* Unsigned wrap is intentional in this reproducible corpus. */
			seed = seed * 1664525u + 1013904223u;
			raw[j] = seed >> 24;
		}
		source.data = raw;
		source.len = source.size = sizeof(raw);
		normalized = fb_UStrFromBytes( &source, FB_STRSIZEVARLEN );
		again = fb_UStrFromBytes( normalized, FB_STRSIZEVARLEN );
		/* Assignment consumes the second temporary, and handles self-aliases. */
		fb_UStrAssign( &dst, -1, again, -1, 0 );
		length = fb_UStrLen( &dst, -1 );
		assert( length > 0 );
		fb_UStrAssign( &dst, -1, &dst, -1, 0 );
		assert( fb_UStrLen( &dst, -1 ) == length );
		fb_UStrAssignMid( &dst, 2, 3, &dst );
		assert( fb_UStrLen( &dst, -1 ) == length );
		fb_UStrSetIndex( &dst, 0, 0x1F600 );
		assert( fb_UStrIndex( &dst, 0 ) == 0x1F600 );
		assert( fb_UStrLen( &dst, -1 ) == length );
		assert( fb_UStrInstr( 1, &dst, &dst ) == 1 );
		assert( fb_UStrInstrRev( &dst, &dst, -1 ) == 1 );
		checkBytes( fb_UStrMid( &dst, 0, 1 ), "", 0 );
		checkBytes( fb_UStrMid( &dst, FB_STRSIZEMSK, FB_STRSIZEMSK ), "", 0 );
	}
	fb_StrDelete( &dst );
	for( i = 0; i < 1000; ++i ) {
		FBSTRING *temporary = fb_UStrFromBytes( "A", 2 );
		fb_UStrConcatAssign( temporary, -1, temporary, -1, 0 );
		assert( FB_ISTEMP(temporary) );
		assert( FB_STRSIZE(temporary) == 2 );
		fb_UStrAssignMid( temporary, 1, 1, fb_UStrFromBytes( "B", 2 ) );
		assert( FB_ISTEMP(temporary) );
		checkBytes( temporary, "BA", 2 );
		assert( fb_UStrConcatAssign( NULL, -1, fb_UStrFromBytes( "A", 2 ), -1, 0 ) == NULL );
	}
}

static void checkWide( void )
{
	FB_WCHAR wide[5], invalid[] = { 0xD800, 65, 0 };
	FB_WCHAR *converted;
	FBSTRING *text;
	wide[0] = 65;
	wide[1] = 0xE9;
	if( sizeof(FB_WCHAR) == 2 ) {
		wide[2] = 0xD83D;
		wide[3] = 0xDE00;
		wide[4] = 0;
	} else {
		wide[2] = (FB_WCHAR)0x1F600;
		wide[3] = 0;
	}
	text = fb_UStrFromWstr( wide );
	assert( fb_UStrLen( text, -1 ) == 3 );
	text = fb_UStrFromWstr( wide );
	converted = fb_UStrToWstr( text );
	assert( converted != NULL );
	assert( converted[0] == 65 );
	assert( converted[1] == 0xE9 );
	if( sizeof(FB_WCHAR) == 2 ) {
		assert( converted[2] == 0xD83D );
		assert( converted[3] == 0xDE00 );
		assert( converted[4] == 0 );
	} else {
		assert( converted[2] == 0x1F600 );
		assert( converted[3] == 0 );
	}
	free( converted );
	checkBytes( fb_UStrFromWstr( invalid ), "\xEF\xBF\xBD" "A", 4 );
}

static void checkOptional( void )
{
	FBSTRING a, b;
	char source[4], mapped[12] = { 0 }, actual[12];
	size_t i;
	ssize_t input_length, output_length, width;
	unsigned int j;
	for( i = 0; i < sizeof(fold_map) / sizeof(*fold_map); ++i ) {
		/* Each mapping contains one to three UTF-8 scalars, at most four bytes each. */
		assert( fold_map[i].count > 0 && fold_map[i].count <= sizeof(mapped) / 4 );
		input_length = fb_hUtf8Encode(source, fold_map[i].scalar);
		output_length = 0;
		for( j = 0; j < fold_map[i].count; ++j )
			output_length += fb_hUtf8Encode(mapped + output_length, fold_map[i].mapped[j]);
		width = fb_hUtf8Fold(fold_map[i].scalar, actual);
		assert( width == output_length );
		assert( memcmp(actual, mapped, width) == 0 );
		a = (FBSTRING){ source, input_length, input_length };
		b = (FBSTRING){ mapped, output_length, output_length };
		assert( fb_UStrComp(&a, &b, 1) == 0 );
	}
	checkBytes(fb_UStrReverse(fb_UStrFromBytes("A\xC3\xA9\xF0\x9F\x98\x80", 8)),
		"\xF0\x9F\x98\x80\xC3\xA9" "A", 7);
	checkBytes(fb_UStrReplace(fb_UStrFromBytes("Stra\xC3\x9F" "e", 8),
		fb_UStrFromBytes("STRASSE", 8), fb_UStrFromBytes("X", 2), 1, -1, 1), "X", 1);
	checkBytes(fb_UStrReplace(fb_UStrFromBytes("A\0B", FB_STRISFIXED | 3),
		fb_UStrFromBytes("\0", FB_STRISFIXED | 1), fb_UStrFromBytes("X", 2), 1, -1, 0), "AXB", 3);
	assert( fb_UStrComp(NULL, NULL, 0) == 0 );
	assert( fb_UStrComp(NULL, NULL, 1) == 0 );
}

static void checkLimits( void )
{
	checkBytes( fb_UStrFill1( FB_STRSIZEMSK, 0x1F600 ), "", 0 );
	assert( fb_ErrorGetNum() == FB_RTERROR_OUTOFMEM );
	checkBytes( fb_UStrFill1( -1, 65 ), "", 0 );
	assert( fb_UStrIndex( NULL, FB_STRSIZEMSK ) == 0 );
	assert( fb_UStrAsc( NULL, 1 ) == 0 );
	assert( fb_UStrInstr( 1, NULL, NULL ) == 0 );
	assert( fb_UStrInstrRevAny( NULL, NULL, -1 ) == 0 );
	checkBytes( fb_UStrTrim( NULL ), "", 0 );
	checkBytes( fb_UStrLcase( NULL, 0 ), "", 0 );
}

static void checkData( void )
{
	FB_WCHAR wide[] = { 0xE9, 0x4E2D, 0 };
	FB_DATADESC data[3];
	FBSTRING dst = { NULL, 0, 0 };
	data[0].len = 3;
	data[0].zstr = "A\0B";
	data[1].len = (short)(FB_DATATYPE_WSTR | 2);
	data[1].wstr = wide;
	data[2].len = FB_DATATYPE_LINK;
	data[2].next = NULL;
	fb_DataRestore( data );
	fb_UStrDataRead( &dst, -1, 0 );
	assert( FB_STRSIZE(&dst) == 3 );
	assert( memcmp( dst.data, "A\0B", 3 ) == 0 );
	fb_UStrDataRead( &dst, -1, 0 );
	assert( FB_STRSIZE(&dst) == 5 );
	assert( memcmp( dst.data, "\xC3\xA9\xE4\xB8\xAD", 5 ) == 0 );
	fb_UStrDataRead( &dst, -1, 0 );
	assert( FB_STRSIZE(&dst) == 0 );
	fb_StrDelete( &dst );
}

static void checkFileIO( void )
{
	static const char *encodings[] = { "utf-8", "utf-16", "utf-32" };
	static const char *invalid[] = {
		"\xE2\x82" "A\r\xF0\x9F\x98\x80\n",
		"\0\xD8\x41\0\r\0\x3D\xD8\0\xDE\n\0",
		"\0\0\x11\0\x41\0\0\0\r\0\0\0\0\xF6\x01\0\n\0\0\0",
	};
	static const size_t lengths[] = { 9, 12, 20 };
	static const char *expected[] = {
		"A\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80",
		"A\0\xE9\0\x2D\x4E\x3D\xD8\0\xDE",
		"A\0\0\0\xE9\0\0\0\x2D\x4E\0\0\0\xF6\x01\0",
	};
	char filename[] = "ustring-runtime-test.tmp";
	char sample[] = "A\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80";
	FBSTRING path = { filename, sizeof(filename) - 1, sizeof(filename) - 1 };
	FBSTRING source = { sample, sizeof(sample) - 1, sizeof(sample) - 1 };
	FBSTRING dst = { NULL, 0, 0 };
	size_t i;
	FILE *raw;
	char actual[16];
	size_t bytes;
	/* The linked host archive has its native wchar ABI. Short-wchar builds
	   exercise our converters and DATA separately, without calling the
	   host archive's wide file putback implementation with foreign units. */
	if( sizeof(FB_WCHAR) != 4 ) return;
	for( i = 0; i < sizeof(encodings) / sizeof(*encodings); ++i ) {
		assert( fb_FileOpenEncod( &path, FB_FILE_MODE_OUTPUT, 0, 0, 1, 0, encodings[i] ) == 0 );
		fb_UStrPrint( 1, &source, FB_PRINT_NEWLINE );
		fb_UStrWrite( 1, &source, FB_PRINT_NEWLINE );
		assert( fb_FileClose(1) == 0 );
		/* A paired ANSI encode/decode can falsely pass a round trip. Check
		   the physical bytes as well, independently of our Unicode reader. */
		raw = fopen( filename, "rb" );
		assert( raw != NULL );
		assert( fseek( raw, i == 0 ? 3 : i == 1 ? 2 : 4, SEEK_SET ) == 0 );
		bytes = i == 2 ? 16 : 10;
		assert( fread( actual, 1, bytes, raw ) == bytes );
		assert( memcmp( actual, expected[i], bytes ) == 0 );
		fclose( raw );
		assert( fb_FileOpenEncod( &path, FB_FILE_MODE_INPUT, 0, 0, 1, 0, encodings[i] ) == 0 );
		assert( fb_UStrFileLineInput( 1, &dst, -1, 0 ) == 0 );
		assert( FB_STRSIZE(&dst) == FB_STRSIZE(&source) );
		assert( memcmp( dst.data, sample, sizeof(sample) - 1 ) == 0 );
		assert( fb_FileInput(1) == 0 );
		assert( fb_UStrInput( &dst, -1, 0 ) == 0 );
		assert( FB_STRSIZE(&dst) == FB_STRSIZE(&source) );
		assert( memcmp( dst.data, sample, sizeof(sample) - 1 ) == 0 );
		assert( fb_FileClose(1) == 0 );
		assert( fb_FileOpenEncod( &path, FB_FILE_MODE_OUTPUT, 0, 0, 1, 0, encodings[i] ) == 0 );
		assert( fwrite( invalid[i], 1, lengths[i], (FILE *)FB_FILE_TO_HANDLE(1)->opaque ) == lengths[i] );
		assert( fb_FileClose(1) == 0 );
		assert( fb_FileOpenEncod( &path, FB_FILE_MODE_INPUT, 0, 0, 1, 0, encodings[i] ) == 0 );
		assert( fb_UStrFileLineInput( 1, &dst, -1, 0 ) == 0 );
		assert( FB_STRSIZE(&dst) == 4 );
		assert( memcmp( dst.data, "\xEF\xBF\xBD" "A", 4 ) == 0 );
		assert( fb_UStrFileLineInput( 1, &dst, -1, 0 ) == 0 );
		assert( FB_STRSIZE(&dst) == 4 );
		assert( memcmp( dst.data, "\xF0\x9F\x98\x80", 4 ) == 0 );
		assert( fb_FileClose(1) == 0 );
	}
	fb_StrDelete( &dst );
	assert( remove(filename) == 0 );
}

int main( int argc, char **argv )
{
	fb_Init( argc, argv, 0 );
	checkScalars();
	checkMalformed();
	checkMaps( lower_map, sizeof(lower_map) / sizeof(*lower_map), FB_TRUE );
	checkMaps( upper_map, sizeof(upper_map) / sizeof(*upper_map), FB_FALSE );
	checkOwnership();
	checkWide();
	checkOptional();
	checkData();
	checkFileIO();
	checkLimits();
	puts( "USTRING runtime: all scalars, case mappings, malformed input, ownership, I/O, and limits passed" );
	return 0;
}

/* end of ustring-runtime.c */
