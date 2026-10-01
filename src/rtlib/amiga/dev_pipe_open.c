/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/dev_pipe_open.c
    Purpose: Define OPEN PIPE's boundary on the pinned Amiga runtime.
    Responsibilities: Report unavailable command-backed streaming explicitly.
    This file intentionally does NOT contain shell or listing-command emulation.

    Synchronous temporary-file capture changes streaming and child lifetime
    semantics. The initial port does not substitute that for popen.
*/

#include "../fb.h"

int fb_DevPipeOpen(FB_FILE *handle, const char *filename, size_t length)
{
    (void)handle; (void)filename; (void)length;
    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

/* end of amiga/dev_pipe_open.c */
