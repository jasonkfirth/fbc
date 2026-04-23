/*
    FreeBASIC runtime support
    -------------------------

    File: io_serial.c

    Purpose:

        Provide serial-port runtime stubs for the Haiku target.

    Responsibilities:

        • satisfy the COM device runtime entry points
        • fail unsupported operations in a defined and consistent way

    This file intentionally does NOT contain:

        • native Haiku serial-port implementation
        • device discovery
        • asynchronous I/O support

    Notes:

        The generic COM device layer is always linked into libfb.
        Platforms that do not yet implement serial support must still
        export these symbols so unsupported COM access fails at runtime
        instead of producing unresolved linker references.
*/

#ifndef DISABLE_HAIKU

#include "../fb.h"

int fb_SerialOpen
    (
        FB_FILE *handle,
        int iPort,
        FB_SERIAL_OPTIONS *options,
        const char *pszDevice,
        void **ppvHandle
    )
{
    (void)handle;
    (void)iPort;
    (void)options;
    (void)pszDevice;
    (void)ppvHandle;

    return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialGetRemaining( FB_FILE *handle, void *pvHandle, fb_off_t *pLength )
{
    (void)handle;
    (void)pvHandle;
    (void)pLength;

    return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialWrite( FB_FILE *handle, void *pvHandle, const void *data, size_t length )
{
    (void)handle;
    (void)pvHandle;
    (void)data;
    (void)length;

    return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialRead( FB_FILE *handle, void *pvHandle, void *data, size_t *pLength )
{
    (void)handle;
    (void)pvHandle;
    (void)data;
    (void)pLength;

    return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

int fb_SerialClose( FB_FILE *handle, void *pvHandle )
{
    (void)handle;
    (void)pvHandle;

    return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

#endif

/* end of io_serial.c */
