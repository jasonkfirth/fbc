/*
 * Project: FreeBASIC runtime tests
 * File: utf-word-file-io.c
 * Purpose: Verify real UTF-16/UTF-32 file paths through normal runtime archives.
 * Responsibilities: Check narrow, wide and USTRING writes, golden bytes and
 *     wide reads using the actual file drivers.
 * This file intentionally does NOT replace drivers or load images.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "fb.h"
#ifdef __cplusplus
}
#endif

#define CHECK(condition) do { if( !(condition) ) { \
	printf( "UTF word file failure at line %d\n", __LINE__ ); return 1; \
} } while( 0 )

static int check_file( char *path, const char *encoding, int writer,
	const unsigned char *expected, size_t expected_size )
{
	FBSTRING filename;
	char utf8[] = "A\xce\xa9\xe2\x82\xac";
	FBSTRING text = {utf8, 6, 0};
	const FB_WCHAR wide[4] = {0x0041, 0x03a9, 0x20ac, 0};
	const FB_WCHAR narrow_wide[3] = {'A', 'z', 0};
	FB_WCHAR readback[8] = {0};
	unsigned char bytes[32];
	FILE *file;
	size_t count;

	filename.data = path;
	filename.len = (ssize_t)strlen( path );
	filename.size = 0;
	CHECK( fb_FileOpenEncod( &filename, FB_FILE_MODE_OUTPUT, FB_FILE_ACCESS_WRITE,
		FB_FILE_LOCK_SHARED, 1, 0, encoding ) == FB_RTERROR_OK );
	if( writer == 0 )
		CHECK( fb_hFilePrintBufferEx( FB_FILE_TO_HANDLE(1), "Az", 2 ) == FB_RTERROR_OK );
	else if( writer == 1 )
		CHECK( fb_hFilePrintBufferWstrEx( FB_FILE_TO_HANDLE(1), wide, 3 ) == FB_RTERROR_OK );
	else {
		fb_UStrPrint( 1, &text, 0 );
		CHECK( fb_ErrorGetNum() == FB_RTERROR_OK );
	}
	CHECK( fb_FileClose( 1 ) == FB_RTERROR_OK );
	file = fopen( path, "rb" );
	CHECK( file != NULL );
	count = fread( bytes, 1, sizeof( bytes ), file );
	CHECK( fclose( file ) == 0 );
	CHECK( count == expected_size && memcmp( bytes, expected, count ) == 0 );
	CHECK( fb_FileOpenEncod( &filename, FB_FILE_MODE_INPUT, FB_FILE_ACCESS_READ,
		FB_FILE_LOCK_SHARED, 1, 0, encoding ) == FB_RTERROR_OK );
	CHECK( fb_FileLineInputWstr( 1, readback, 8 ) == FB_RTERROR_OK );
	if( writer == 0 ) CHECK( memcmp( readback, narrow_wide, sizeof( narrow_wide ) ) == 0 );
	else CHECK( memcmp( readback, wide, sizeof( wide ) ) == 0 );
	CHECK( fb_FileClose( 1 ) == FB_RTERROR_OK );
	CHECK( remove( path ) == 0 );
	return 0;
}

int main( int argc, char **argv )
{
	/* BOMs and payloads are golden bytes, independent of the helpers under test. */
	static const unsigned char narrow16[] = {0xff,0xfe, 0x41,0x00, 0x7a,0x00};
	static const unsigned char wide16[] = {0xff,0xfe, 0x41,0x00, 0xa9,0x03, 0xac,0x20};
	static const unsigned char narrow32[] = {0xff,0xfe,0x00,0x00, 0x41,0x00,0x00,0x00, 0x7a,0x00,0x00,0x00};
	static const unsigned char wide32[] = {0xff,0xfe,0x00,0x00, 0x41,0x00,0x00,0x00, 0xa9,0x03,0x00,0x00, 0xac,0x20,0x00,0x00};
	int writer;

	CHECK( argc == 2 );
	fb_Init( argc, argv, 0 );
	for( writer = 0; writer < 3; ++writer ) {
		CHECK( check_file( argv[1], "utf16", writer, writer ? wide16 : narrow16,
			writer ? sizeof( wide16 ) : sizeof( narrow16 ) ) == 0 );
		CHECK( check_file( argv[1], "utf32", writer, writer ? wide32 : narrow32,
			writer ? sizeof( wide32 ) : sizeof( narrow32 ) ) == 0 );
	}
	puts( "UTF_WORD_FILE_IO_PASS cases=6" );
	return 0;
}

/* end of utf-word-file-io.c */
