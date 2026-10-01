/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/file_hseteof.c
    Purpose: Resize a file to its current logical stdio position.
    Responsibilities: Check DOS offset limits and refresh stdio after resizing.
    This file intentionally does NOT contain file-number or buffering policy.
*/

#include "../fb.h"
#include <proto/dos.h>

int fb_hFileSetEofEx(FILE *stream)
{
    unsigned long handle;
    fb_off_t position = ftello(stream);
    if (position < 0 || position > LONG_MAX ||
        fb_hAmigaGetFileHandle(stream, &handle) != 0 ||
        SetFileSize((BPTR)handle, (LONG)position, OFFSET_BEGINNING) == -1 ||
        fseeko(stream, position, SEEK_SET) != 0)
        return fb_ErrorSetNum(FB_RTERROR_FILEIO);
    clearerr(stream);
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of amiga/file_hseteof.c */
