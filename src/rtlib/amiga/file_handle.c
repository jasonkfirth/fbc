/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/file_handle.c
    Purpose: Resolve the pinned newlib descriptor table at one native boundary.
    Responsibilities: Validate stream descriptors and return borrowed DOS handles.
    This file intentionally does NOT contain stream positioning or ownership.

    The pinned SDK's open.c exports __fh and __maxfh. FILE stores an index into
    that table, not a BPTR. Keep this SDK dependency here; generic file code
    must never reinterpret a descriptor as a native handle. Callers serialize
    FreeBASIC file operations with FB_LOCK and do not close the returned handle.
*/

#include "../fb.h"
#include <proto/dos.h>

extern BPTR *__fh;
extern int __maxfh;

int fb_hAmigaGetFileHandle(FILE *stream, unsigned long *handle)
{
    int descriptor;
    BPTR result;

    if (stream == NULL || handle == NULL) return -1;
    descriptor = fileno(stream);
    if (descriptor < 0 || descriptor >= __maxfh || __fh == NULL) return -1;
    if (descriptor == 0) result = Input();
    else if (descriptor == 1 || descriptor == 2) result = Output();
    else result = __fh[descriptor];
    if (result == 0) return -1;
    *handle = (unsigned long)result;
    return 0;
}

/* end of amiga/file_handle.c */
