/*
 * Project: FreeBASIC runtime qualification
 * File: string-get-runtime.c
 * Purpose: Check string GET through a rebuilt normal runtime archive.
 * Responsibilities: Use actual file open/read/close calls and verify fixed
 *     STRING storage, ZSTRING termination and descriptor-backed destinations.
 * This file intentionally does NOT provide runtime or file-driver stubs.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "fb.h"
#ifdef __cplusplus
}
#endif

#define CHECK(condition) do { if( !(condition) ) { \
    printf( "archive string GET failure at line %d\n", __LINE__ ); return 1; \
} } while( 0 )

int main( int argc, char **argv )
{
    FILE *output;
    char data[] = { 'x', 'x', 'x', 'Q' };
    FBSTRING filename;
    FBSTRING descriptor = {data, 3, 3};
    size_t bytes = 0;
    const ssize_t fixed = (ssize_t)((size_t)FB_STRISFIXED | (size_t)3);

    CHECK( argc == 2 );
    output = fopen( argv[1], "wb" );
    CHECK( output != NULL );
    CHECK( fwrite( "abc", 1, 3, output ) == 3 );
    CHECK( fclose( output ) == 0 );

    fb_Init( argc, argv, 0 );
    filename.data = argv[1];
    filename.len = (ssize_t)strlen( argv[1] );
    filename.size = 0;
    CHECK( fb_FileOpen( &filename, FB_FILE_MODE_BINARY, FB_FILE_ACCESS_READ,
                       FB_FILE_LOCK_SHARED, 1, 0 ) == FB_RTERROR_OK );
    CHECK( fb_FileGetStrIOB( 1, 1, data, fixed, &bytes ) == FB_RTERROR_OK );
    CHECK( bytes == 3 && memcmp( data, "abc", 3 ) == 0 && data[3] == 'Q' );

    CHECK( fb_FileGetStrLargeIOB( 1, 1, data, 4, &bytes ) == FB_RTERROR_OK );
    /* A missing terminator must fail the test without an unbounded read. */
    CHECK( bytes == 3 && memcmp( data, "abc", 4 ) == 0 );
    data[3] = 'Q';
    CHECK( fb_FileGetStrIOB( 1, 1, &descriptor, FB_STRSIZEVARLEN, &bytes ) == FB_RTERROR_OK );
    CHECK( bytes == 3 && data[3] == '\0' && descriptor.len == 3 );
    CHECK( fb_FileClose( 1 ) == FB_RTERROR_OK );
    CHECK( remove( argv[1] ) == 0 );

    puts( "STRING_GET_RUNTIME_PASS" );
    return 0;
}

/* end of string-get-runtime.c */
