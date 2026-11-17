/*
 * Project: FreeBASIC runtime tests
 * File: rtlib/com-option-ranges.c
 * Purpose: Check numeric OPEN COM options before platform dispatch.
 * Responsibilities: Preserve defaults and valid values, reject overflow, and
 *     check that failed parsing leaves the platform unopened.
 * Platform serial operations belong to the observer functions below.
 */
#include "fb.h"
#include <limits.h>

#ifndef FB_COM_OPTION_SOURCE
#define FB_COM_OPTION_SOURCE "../../src/rtlib/devices/dev_com.c"
#endif
#include FB_COM_OPTION_SOURCE
#include "../../src/rtlib/devices/dev_com_test.c"

FB_RTLIB_CTX fb_ctx;
static FB_SERIAL_OPTIONS observed;
static int opened, checks, error_number;

static void check( int condition )
{
    ++checks;
    if( !condition ) {
        fprintf( stderr, "COM_OPTION_RANGES_FAIL check=%d\n", checks );
        exit( 1 );
    }
}

FBCALL int fb_ErrorSetNum( int error )
{
    error_number = error;
    return error;
}

int fb_SerialOpen( FB_FILE *handle, int port, FB_SERIAL_OPTIONS *options,
                   const char *device, void **serial_handle )
{
    (void)handle;
    (void)port;
    (void)device;
    observed = *options;
    ++opened;
    *serial_handle = &observed;
    return FB_RTERROR_OK;
}

int fb_SerialClose( FB_FILE *handle, void *serial_handle )
{
    (void)handle;
    (void)serial_handle;
    return FB_RTERROR_OK;
}

int fb_SerialWrite( FB_FILE *handle, void *serial_handle, const void *data, size_t length )
{
    (void)handle;
    (void)serial_handle;
    (void)data;
    (void)length;
    return FB_RTERROR_OK;
}

int fb_SerialRead( FB_FILE *handle, void *serial_handle, void *data, size_t *length )
{
    (void)handle;
    (void)serial_handle;
    (void)data;
    *length = 0;
    return FB_RTERROR_OK;
}

int fb_SerialGetRemaining( FB_FILE *handle, void *serial_handle, fb_off_t *length )
{
    (void)handle;
    (void)serial_handle;
    *length = 0;
    return FB_RTERROR_OK;
}

static unsigned int option_value( size_t index )
{
    switch( index ) {
    case 0: return observed.uiSpeed;
    case 1: return observed.uiDataBits;
    case 2: return observed.DurationCTS;
    case 3: return observed.DurationDSR;
    case 4: return observed.DurationCD;
    case 5: return observed.OpenTimeout;
    case 6: return observed.TransmitBuffer;
    case 7: return observed.ReceiveBuffer;
    case 8: return observed.IRQNumber;
    default: abort();
    }
}

static void option( size_t index, const char *number, int valid, unsigned int value )
{
    static const char *const prefixes[] = {
        "COM1:", "COM1:300,N,", "COM1:300,N,8,1,CS", "COM1:300,N,8,1,DS",
        "COM1:300,N,8,1,CD", "COM1:300,N,8,1,OP", "COM1:300,N,8,1,TB",
        "COM1:300,N,8,1,RB", "COM1:300,N,8,1,IR"
    };
    const char *suffix = index == 0 ? ",N,8,1" : index == 1 ? ",1" : "";
    FB_FILE handle;
    char text[256];
    int result;
    memset( &handle, 0, sizeof(handle) );
    handle.mode = FB_FILE_MODE_BINARY;
    opened = error_number = 0;
    snprintf( text, sizeof(text), "%s%s%s", prefixes[index], number, suffix );
    result = fb_DevComOpen( &handle, text, strlen(text) );
    check( (result == FB_RTERROR_OK) == valid );
    check( opened == valid );
    if( valid ) {
        check( option_value(index) == value );
        check( handle.hooks->pfnClose(&handle) == FB_RTERROR_OK );
    } else {
        check( error_number == FB_RTERROR_ILLEGALFUNCTIONCALL );
        check( handle.hooks == NULL && handle.opaque == NULL );
    }
}

int main( void )
{
    char maximum[32], overflow[32], prefixed[40];
    size_t index;
    snprintf( maximum, sizeof(maximum), "%u", UINT_MAX );
    snprintf( overflow, sizeof(overflow), "%llu", (unsigned long long)UINT_MAX + 1 );
    for( index = 0; index < 9; ++index ) {
        option( index, "8", TRUE, 8 );
        option( index, "0", TRUE, 0 );
        option( index, "-0", TRUE, 0 );
        option( index, "+8", TRUE, 8 );
        option( index, "  +0008 ", TRUE, 8 );
        option( index, maximum, TRUE, UINT_MAX );
        snprintf( prefixed, sizeof(prefixed), "+%s", maximum );
        option( index, prefixed, TRUE, UINT_MAX );
        option( index, "", TRUE, index == 0 ? 300U : index == 1 ? 7U : 0U );
        option( index, overflow, FALSE, 0 );
        option( index, "184467440737095516160000", FALSE, 0 );
        option( index, "-1", FALSE, 0 );
        option( index, "  -8", FALSE, 0 );
        option( index, "8junk", FALSE, 0 );
    }
    printf( "COM_OPTION_RANGES_PASS checks=%d\n", checks );
    return 0;
}

/* end of rtlib/com-option-ranges.c */
