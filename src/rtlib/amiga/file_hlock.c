/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/file_hlock.c
    Purpose: Adapt stream record locks to the AmigaDOS record-lock API.
    Responsibilities: Check offset bounds and use borrowed native file handles.
    This file intentionally does NOT contain a guessed native stream layout.

    Use immediate exclusive locks so a conflicting lock does not stop an
    unattended program indefinitely. The descriptor bridge is SDK-specific.
*/

#include "../fb.h"
#include <proto/dos.h>

static int valid_range(fb_off_t position, fb_off_t size)
{
    return position >= 0 && size >= 0 && position <= LONG_MAX &&
        size <= LONG_MAX - position;
}

int fb_hFileLock(FILE *stream, fb_off_t position, fb_off_t size)
{
    unsigned long handle;
    if (!valid_range(position, size))
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    if (fb_hAmigaGetFileHandle(stream, &handle) != 0 ||
        !LockRecord((BPTR)handle, (ULONG)position, (ULONG)size,
            REC_EXCLUSIVE_IMMED, 0))
        return fb_ErrorSetNum(FB_RTERROR_FILEIO);
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_hFileUnlock(FILE *stream, fb_off_t position, fb_off_t size)
{
    unsigned long handle;
    if (!valid_range(position, size))
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    if (fb_hAmigaGetFileHandle(stream, &handle) != 0 ||
        !UnLockRecord((BPTR)handle, (ULONG)position, (ULONG)size))
        return fb_ErrorSetNum(FB_RTERROR_FILEIO);
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of amiga/file_hlock.c */
