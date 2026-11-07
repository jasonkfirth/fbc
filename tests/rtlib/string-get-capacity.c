/*
 * Project: FreeBASIC runtime qualification
 * File: string-get-capacity.c
 * Purpose: Exercise production string GET across its signed-length boundary.
 * Responsibilities: Check requested byte counts, short reads, descriptor
 *     retirement and the distinct terminator rules for STRING and ZSTRING.
 * This file intentionally does NOT implement a file driver or allocate buffers.
 */
#include "fb.h"

FB_RTLIB_CTX fb_ctx;
static size_t requested, supplied;
static int reads, retired;

FBCALL int fb_ErrorSetNum( int number )
{
    return number;
}

FBCALL int fb_hStrDelTemp( FBSTRING_ACCESS *string )
{
    if( string != NULL )
        ++retired;
    return 0;
}

/* Keep the byte-count contract of the actual file layer. Supplying fewer bytes
 * than requested exercises the caller's result count and terminator handling. */
int fb_FileGetDataEx( FB_FILE *handle, fb_off_t position, void *data,
                     size_t length, size_t *bytesread, int adjust, int unicode )
{
    size_t count = supplied < length ? supplied : length;
    (void)handle;
    (void)position;
    (void)adjust;
    (void)unicode;
    requested = length;
    ++reads;
    memset( data, 'Z', count );
    *bytesread = count;
    return FB_RTERROR_OK;
}

#define CHECK(condition) do { if( !(condition) ) { \
    printf( "string GET failure at line %d\n", __LINE__ ); return 1; \
} } while( 0 )

int main( void )
{
    FB_FILE handle = {0};
    FB_FILE_HOOKS hooks = {0};
    char data[] = { 'a', 'b', 'c', 'Q' };
    FBSTRING descriptor = {data, 3, 3};
    size_t bytes;
    const ssize_t fixed = (ssize_t)((size_t)FB_STRISFIXED | (size_t)3);
    handle.hooks = &hooks;

    supplied = 3;
    CHECK( fb_FileGetStrEx( &handle, 0, data, fixed, &bytes ) == FB_RTERROR_OK );
    CHECK( requested == 3 && bytes == 3 && reads == 1 );
    CHECK( data[0] == 'Z' && data[2] == 'Z' && data[3] == 'Q' );

    supplied = 2;
    CHECK( fb_FileGetStrEx( &handle, 0, &descriptor, FB_STRSIZEVARLEN, &bytes ) == FB_RTERROR_OK );
    CHECK( requested == 3 && bytes == 2 && retired == 1 );
    CHECK( data[2] == '\0' && data[3] == 'Q' && descriptor.len == 3 );

    memcpy( data, "abc", 4 );
    CHECK( fb_FileGetStrEx( &handle, 0, data, 4, &bytes ) == FB_RTERROR_OK );
    CHECK( requested == 3 && bytes == 2 && data[2] == '\0' );

    memcpy( data, "abc", 4 );
    supplied = 3;
    CHECK( fb_FileGetStrEx( &handle, 0, data, 0, NULL ) == FB_RTERROR_OK );
    CHECK( requested == 3 && data[3] == '\0' && reads == 4 );

    bytes = 99;
    CHECK( fb_FileGetStrEx( NULL, 0, data, 4, &bytes ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
    CHECK( bytes == 0 && reads == 4 );
    data[0] = '\0';
    CHECK( fb_FileGetStrEx( &handle, 0, data, 1, &bytes ) == FB_RTERROR_ILLEGALFUNCTIONCALL );
    CHECK( bytes == 0 && reads == 4 );

    puts( "STRING_GET_CAPACITY_PASS" );
    return 0;
}

/* end of string-get-capacity.c */
