/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/file_hflush.c
    Purpose: Flush native file buffers after stdio has flushed its own buffer.
    Responsibilities: Adapt DOS Flush to the runtime error contract.
    This file intentionally does NOT contain stream ownership or POSIX fsync.
*/

#include "../fb.h"
#include <proto/dos.h>

int fb_hFileFlushEx(FILE *stream)
{
    unsigned long handle;
    if (fb_hAmigaGetFileHandle(stream, &handle) != 0 || !Flush((BPTR)handle))
        return fb_ErrorSetNum(FB_RTERROR_FILEIO);
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of amiga/file_hflush.c */
