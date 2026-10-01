/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/io_printer.c
    Purpose: Send LPRINT output to the configured AmigaDOS PRT handler.
    Responsibilities: Own the native printer handle and convert bounded text.
    This file intentionally does NOT contain Unix print-command construction.

    This uses the native printer path from FreeBASIC-NG's Amiga backend.
    Opening fails normally when the guest has no configured PRT handler.
*/

#include "../fb.h"
#include <proto/dos.h>

int fb_PrinterOpen(DEV_LPT_INFO *device, int port, const char *name)
{
    BPTR handle;
    (void)name;
    if (device == NULL || port > 1 || port < 0)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    handle = Open("PRT:", MODE_NEWFILE);
    if (handle == 0) return fb_ErrorSetNum(FB_RTERROR_FILENOTFOUND);
    device->driver_opaque = (void *)(uintptr_t)handle;
    device->iPort = port;
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_PrinterWrite(DEV_LPT_INFO *device, const void *data, size_t length)
{
    BPTR handle;
    const unsigned char *cursor = data;
    if (device == NULL || (data == NULL && length != 0))
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    handle = (BPTR)(uintptr_t)device->driver_opaque;
    if (handle == 0) return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    while (length != 0) {
        LONG requested = length > LONG_MAX ? LONG_MAX : (LONG)length;
        LONG written = Write(handle, (APTR)cursor, requested);
        if (written <= 0) return fb_ErrorSetNum(FB_RTERROR_FILEIO);
        cursor += written; length -= (size_t)written;
    }
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_PrinterWriteWstr(DEV_LPT_INFO *device, const FB_WCHAR *data, size_t length)
{
    char buffer[256];
    size_t index, count;
    int result;
    if (data == NULL && length != 0)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    while (length != 0) {
        count = length < sizeof(buffer) ? length : sizeof(buffer);
        for (index = 0; index < count; ++index)
            buffer[index] = data[index] < 256 ? (char)data[index] : '?';
        result = fb_PrinterWrite(device, buffer, count);
        if (result != FB_RTERROR_OK) return result;
        data += count; length -= count;
    }
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_PrinterClose(DEV_LPT_INFO *device)
{
    BPTR handle;
    if (device == NULL) return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    handle = (BPTR)(uintptr_t)device->driver_opaque;
    device->driver_opaque = NULL;
    if (handle != 0 && !Close(handle)) return fb_ErrorSetNum(FB_RTERROR_FILEIO);
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of amiga/io_printer.c */
