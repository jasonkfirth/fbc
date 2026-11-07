/*
 * Project: FreeBASIC runtime qualification
 * File: string-setup-values.c
 * Purpose: Check the read-only descriptor and signed string setup contracts.
 * Responsibilities: Distinguish binary STRING lengths, ZSTRING content lengths
 *     and writable ZSTRING capacity without allocating large buffers.
 * This file intentionally does NOT change or simulate string allocation.
 */
#include "fb.h"

#ifndef FB_TEST_READ_ONLY
static int check_setup( const void *source, ssize_t size,
                        const char *expected, ssize_t content, ssize_t capacity )
{
    const char *pointer;
    ssize_t length;

    FB_STRSETUP_FIX( source, size, pointer, length );
    if( (pointer != expected) || (length != content) )
        return 1;
    FB_STRSETUP_DYN( source, size, pointer, length );
    return (pointer != expected) || (length != capacity);
}
#endif

int main( void )
{
    char text[] = { 'a', 'b', 'c', '\0' };
    const FBSTRING descriptor = {
        text, (ssize_t)((size_t)FB_TEMPSTRBIT | (size_t)3), 3
    };

    /* Both queries read descriptor metadata. They must accept a const view
     * without discarding qualifiers or treating the ownership bit as length. */
    if( !FB_ISTEMP( &descriptor ) || (FB_STRSIZE( &descriptor ) != 3) )
        return 1;

#ifndef FB_TEST_READ_ONLY
    char binary[] = { 'a', '\0', 'b', '\0' };
    char short_text[] = { 'a', '\0', 'b', '\0' };
    char empty[] = { '\0' };
    ssize_t fixed_size = (ssize_t)((size_t)FB_STRISFIXED | (size_t)3);

    if( check_setup( NULL, 0, NULL, 0, 0 ) ||
        check_setup( NULL, FB_STRSIZEVARLEN, NULL, 0, 0 ) ||
        check_setup( &descriptor, FB_STRSIZEVARLEN, text, 3, 3 ) ||
        check_setup( binary, fixed_size, binary, 3, 3 ) ||
        check_setup( text, 0, text, 3, 3 ) ||
        check_setup( text, 4, text, 3, 3 ) ||
        check_setup( short_text, 4, short_text, 1, 3 ) ||
        check_setup( empty, 1, empty, 0, 0 ) )
        return 2;
#endif

    puts( "STRING_SETUP_VALUES_PASS" );
    return 0;
}

/* end of string-setup-values.c */
