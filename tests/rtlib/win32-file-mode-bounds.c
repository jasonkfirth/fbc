/*
 * Project: FreeBASIC runtime tests
 * File: win32-file-mode-bounds.c
 * Purpose: Check Windows CRT mode conversion at its stack-buffer limit.
 * Responsibilities: Preserve valid mode bytes and reject an oversized mode.
 * This file does not create, truncate or reopen files.
 */
#ifndef FB_PATH_CONVERSION_SOURCE
#define FB_PATH_CONVERSION_SOURCE "../../src/rtlib/drivers/win32/file_hconvpath.c"
#endif
#include FB_PATH_CONVERSION_SOURCE
#include <errno.h>

static int checks;

static void check( int condition )
{
    ++checks;
    if( !condition ) {
        fprintf( stderr, "WIN32_FILE_MODE_BOUNDS_FAIL check=%d\n", checks );
        exit( 1 );
    }
}

int main( void )
{
    wchar_t buffer[16];
    const char *modes[] = {"", "r", "rb", "r+b", "wb", "a+b", "rbN", "1234567"};
    size_t i, j;

    for( i = 0; i < sizeof( buffer ) / sizeof( buffer[0] ); ++i ) buffer[i] = 0x55aa;
    fb_hCopyModeToWC( "123456789", buffer, 8 );
    check( buffer[8] == 0x55aa );
    check( buffer[0] == L'\0' && errno == EINVAL );
    for( i = 0; i < sizeof( modes ) / sizeof( modes[0] ); ++i ) {
        for( j = 0; j < sizeof( buffer ) / sizeof( buffer[0] ); ++j ) buffer[j] = 0x55aa;
        errno = 0;
        fb_hCopyModeToWC( modes[i], buffer, 8 );
        for( j = 0; j < strlen( modes[i] ); ++j )
            check( buffer[j] == (wchar_t)(unsigned char)modes[i][j] );
        check( buffer[j] == L'\0' && buffer[8] == 0x55aa && errno == 0 );
    }
    fb_hCopyModeToWC( "r", buffer, 1 );
    check( buffer[0] == L'\0' && errno == EINVAL );
    fb_hCopyModeToWC( NULL, buffer, 8 );
    check( errno == EINVAL );
    fb_hCopyModeToWC( "r", NULL, 8 );
    check( errno == EINVAL );
    fb_hCopyModeToWC( "r", buffer, 0 );
    check( errno == EINVAL );
    printf( "WIN32_FILE_MODE_BOUNDS_PASS checks=%d\n", checks );
    return 0;
}

/* end of win32-file-mode-bounds.c */
