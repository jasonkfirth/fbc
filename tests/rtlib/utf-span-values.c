/*
 * Project: FreeBASIC runtime library tests
 * File: utf-span-values.c
 * Purpose: Check valid UTF conversions, output ownership and signed byte counts.
 * Responsibilities: Fixed Unicode vectors, all Latin-1 bytes and count boundaries.
 * This file intentionally does NOT use devices, files or native wide CRT calls.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fb.h"
#include "strings/encoding/utf_size.h"

typedef union UTF_OUTPUT {
    UTF_32 words[128];
    unsigned char bytes[512];
    FB_WCHAR wide[128];
} UTF_OUTPUT;

static unsigned int checks;
static unsigned int old_defects;

static void check( int condition, const char *name )
{
    if( !condition ) {
        fprintf( stderr, "UTF span check failed: %s (wchar=%u)\n", name, (unsigned int)sizeof( FB_WCHAR ) );
        exit( 1 );
    }
    checks += 1;
}

static void latin1_values( void )
{
    unsigned int value;
    int format;
    for( value = 0; value <= 255; ++value ) {
        char source = (char)value;
        for( format = FB_FILE_ENCOD_UTF8; format <= FB_FILE_ENCOD_UTF32; ++format ) {
            FB_FILE_ENCOD encod = (FB_FILE_ENCOD)format;
            UTF_OUTPUT supplied;
            unsigned char expected[4] = { 0, 0, 0, 0 };
            ssize_t expected_bytes, bytes = -1;
            char *allocated, *returned;
            if( encod == FB_FILE_ENCOD_UTF8 ) {
                expected_bytes = value < 128 ? 1 : 2;
                expected[0] = (unsigned char)(value < 128 ? value : (0xc0 | (value >> 6)));
                if( value >= 128 )
                    expected[1] = (unsigned char)(0x80 | (value & 0x3f));
            } else {
                /* These APIs expose native UTF code units, independently of
                   the byte order used by encoded file I/O. */
                UTF_16 unit16 = (UTF_16)value;
                UTF_32 unit32 = value;
                expected_bytes = encod == FB_FILE_ENCOD_UTF16 ? 2 : 4;
                memcpy( expected, encod == FB_FILE_ENCOD_UTF16 ? (const void *)&unit16 : (const void *)&unit32,
                    (size_t)expected_bytes );
            }
            memset( &supplied, 0x5a, sizeof( supplied ) );
            returned = fb_CharToUTF( encod, &source, 1, (char *)supplied.bytes, &bytes );
            check( returned == (char *)supplied.bytes && bytes == expected_bytes, "Latin-1 supplied count" );
            check( memcmp( supplied.bytes, expected, (size_t)bytes ) == 0 && supplied.bytes[bytes] == 0x5a,
                "Latin-1 supplied payload and boundary" );
            allocated = fb_CharToUTF( encod, &source, 1, NULL, &bytes );
            check( allocated != NULL && bytes == expected_bytes && memcmp( allocated, expected, (size_t)bytes ) == 0,
                "Latin-1 allocated payload" );
            free( allocated );
            if( value != 0 ) {
                UTF_OUTPUT input;
                char narrow[4] = { 0, 0, 0, 0 };
                ssize_t chars = 4;
                memset( &input, 0, sizeof( input ) );
                memcpy( input.bytes, expected, (size_t)expected_bytes );
                check( fb_UTFToChar( encod, input.bytes, narrow, &chars ) == narrow && chars == 1 &&
                    (unsigned char)narrow[0] == value && narrow[1] == 0, "Latin-1 supplied decode" );
                allocated = fb_UTFToChar( encod, input.bytes, NULL, &chars );
                check( allocated != NULL && chars == 1 && (unsigned char)allocated[0] == value && allocated[1] == 0,
                    "Latin-1 allocated decode" );
                free( allocated );
            }
        }
    }
}

static void unicode_values( int after )
{
    static const unsigned char utf8[] = { 0x41, 0xf0, 0x90, 0x80, 0x80, 0x42,
        0xf0, 0x9f, 0x99, 0x82, 0x43, 0 };
    static const UTF_16 utf16[] = { 0x41, 0xd800, 0xdc00, 0x42, 0xd83d, 0xde42, 0x43, 0 };
    static const UTF_32 utf32[] = { 0x41, 0x10000, 0x42, 0x1f642, 0x43, 0 };
    const void *inputs[] = { utf8, utf16, utf32 };
    const ssize_t lengths[] = { 11, 14, 20 };
    FB_WCHAR expected[8];
    ssize_t count, chars;
    unsigned int index;
    int format, supplied;

    count = sizeof( FB_WCHAR ) == 2 ? 7 : 5;
    for( index = 0; index <= (unsigned int)count; ++index ) {
        UTF_32 c = sizeof( FB_WCHAR ) == 2 ? utf16[index] : utf32[index];
        expected[index] = (FB_WCHAR)(sizeof( FB_WCHAR ) == 1 && c > 255 ? '?' : c);
    }
    for( format = FB_FILE_ENCOD_UTF8; format <= FB_FILE_ENCOD_UTF32; ++format ) {
        FB_FILE_ENCOD encod = (FB_FILE_ENCOD)format;
        int slot = format - FB_FILE_ENCOD_UTF8;
        for( supplied = 0; supplied != 2; ++supplied ) {
            UTF_OUTPUT output;
            FB_WCHAR *converted;
            int equal, known_old_defect;
            memset( &output, 0x5a, sizeof( output ) );
            chars = supplied ? 128 : 0;
            converted = fb_UTFToWChar( encod, inputs[slot], supplied ? output.wide : NULL, &chars );
            equal = converted != NULL && chars == count &&
                memcmp( converted, expected, (size_t)(count + 1) * sizeof( FB_WCHAR ) ) == 0;
            known_old_defect = (sizeof( FB_WCHAR ) == 2 && encod == FB_FILE_ENCOD_UTF8) ||
                (sizeof( FB_WCHAR ) == 4 && encod == FB_FILE_ENCOD_UTF16);
            if( !after && known_old_defect ) {
                check( !equal, "baseline supplementary-character defect observed" );
                old_defects += 1;
            } else {
                check( equal, "supplementary scalar sequence decode" );
            }
            if( !supplied )
                free( converted );
        }
        if( sizeof( FB_WCHAR ) != 1 ) {
            ssize_t bytes;
            char *allocated;
            UTF_OUTPUT output;
            allocated = fb_WCharToUTF( encod, expected, count, NULL, &bytes );
            check( allocated != NULL && bytes == lengths[slot] &&
                memcmp( allocated, inputs[slot], (size_t)bytes ) == 0, "supplementary scalar sequence encode" );
            free( allocated );
            /* The old 32-bit-wide UTF16 path reallocates its output parameter.
               Only exercise caller-owned storage after the ownership repair. */
            if( after || sizeof( FB_WCHAR ) != 4 || encod != FB_FILE_ENCOD_UTF16 ) {
                memset( &output, 0x5a, sizeof( output ) );
                check( fb_WCharToUTF( encod, expected, count, (char *)output.bytes, &bytes ) == (char *)output.bytes &&
                    bytes == lengths[slot] && memcmp( output.bytes, inputs[slot], (size_t)bytes ) == 0 &&
                    output.bytes[bytes] == 0x5a, "caller-owned encoding buffer" );
            }
        }
    }
}

static void growth_values( void )
{
    UTF_32 source[41];
    unsigned int index;
    int format;
    for( index = 0; index < 40; ++index )
        source[index] = 65 + index % 26;
    source[40] = 0;
    for( format = FB_FILE_ENCOD_UTF8; format <= FB_FILE_ENCOD_UTF32; ++format ) {
        FB_FILE_ENCOD encod = (FB_FILE_ENCOD)format;
        UTF_OUTPUT input;
        FB_WCHAR *wide;
        char *narrow;
        ssize_t chars;
        for( index = 0; index <= 40; ++index ) {
            if( encod == FB_FILE_ENCOD_UTF8 )
                input.bytes[index] = (unsigned char)source[index];
            else if( encod == FB_FILE_ENCOD_UTF16 )
                ((UTF_16 *)input.bytes)[index] = (UTF_16)source[index];
            else
                input.words[index] = source[index];
        }
        wide = fb_UTFToWChar( encod, input.bytes, NULL, &chars );
        check( wide != NULL && chars == 40 && wide[40] == 0, "wide repeated eight-unit growth" );
        for( index = 0; index < 40; ++index )
            check( (UTF_32)wide[index] == source[index], "wide growth payload" );
        free( wide );
        narrow = fb_UTFToChar( encod, input.bytes, NULL, &chars );
        check( narrow != NULL && chars == 40 && narrow[40] == 0, "narrow repeated eight-unit growth" );
        for( index = 0; index < 40; ++index )
            check( (UTF_32)(unsigned char)narrow[index] == source[index], "narrow growth payload" );
        free( narrow );
    }
}

static void count_values( void )
{
    size_t bytes = 99;
    ssize_t capacity = 8, result;
    const char c = 'A';
    const FB_WCHAR wide = 'A';
    int format;
    check( fb_hUTFBytes( 0, 4, &bytes ) && bytes == 0, "empty count" );
    check( !fb_hUTFBytes( -1, 2, &bytes ), "negative count rejected" );
    check( !fb_hUTFBytes( FB_STRSIZEMSK, 4, &bytes ), "unrepresentable byte count rejected" );
    check( fb_hUTFBytes( FB_STRSIZEMSK / 4, 4, &bytes ) && bytes == (size_t)(FB_STRSIZEMSK / 4) * 4,
        "largest representable four-byte count" );
    check( fb_hUTFGrow( &capacity, 1, 2, &bytes ) && capacity == 15 && bytes == 30,
        "growth reclaims remaining unit" );
    capacity = FB_STRSIZEMSK - 7;
    check( !fb_hUTFGrow( &capacity, 0, 1, &bytes ) && capacity == FB_STRSIZEMSK - 7,
        "growth limit rejected before addition" );
    for( format = FB_FILE_ENCOD_UTF8; format <= FB_FILE_ENCOD_UTF32; ++format ) {
        FB_FILE_ENCOD encod = (FB_FILE_ENCOD)format;
        check( fb_CharToUTF( encod, &c, -1, NULL, &result ) == NULL && result == 0,
            "public negative narrow count" );
        check( fb_WCharToUTF( encod, &wide, -1, NULL, &result ) == NULL && result == 0,
            "public negative wide count" );
        check( fb_CharToUTF( encod, &c, FB_STRSIZEMSK, NULL, &result ) == NULL && result == 0,
            "public narrow byte-count limit" );
        check( fb_WCharToUTF( encod, &wide, FB_STRSIZEMSK, NULL, &result ) == NULL && result == 0,
            "public wide byte-count limit" );
    }
}

int main( int argc, char **argv )
{
    int after;
    check( argc == 2, "phase argument" );
    after = strcmp( argv[1], "after" ) == 0;
    check( after || strcmp( argv[1], "before" ) == 0, "recognized phase" );
    latin1_values();
    unicode_values( after );
    growth_values();
    if( after )
        count_values();
    check( old_defects == (after || sizeof( FB_WCHAR ) == 1 ? 0u : 2u), "expected baseline defect count" );
    printf( "UTF_SPAN_VALUES_PASS width=%u phase=%s checks=%u old_defects=%u\n",
        (unsigned int)sizeof( FB_WCHAR ), argv[1], checks, old_defects );
    return 0;
}

/* end of utf-span-values.c */
