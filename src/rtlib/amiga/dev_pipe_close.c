/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/dev_pipe_close.c
    Purpose: Close a BASIC command stream and reap its native workers.
    Responsibilities: Serialize the registry and clear the released stream.
    This file intentionally does NOT contain packet handling or process creation.
*/

#include "../fb.h"
int fb_hAmigaPipeClose(FILE *stream);

int fb_DevPipeClose(FB_FILE *handle)
{
    if (handle == NULL) return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    FB_LOCK();
    if (handle->opaque != NULL) fb_hAmigaPipeClose(handle->opaque);
    handle->opaque = NULL;
    FB_UNLOCK();
    return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of amiga/dev_pipe_close.c */
