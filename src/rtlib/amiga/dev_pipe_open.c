/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/dev_pipe_open.c
    Purpose: Open a native command stream through normal stdio file hooks.
    Responsibilities: Validate modes, select hooks, and preserve stream ownership.
    This file intentionally does NOT contain packet handling or shell execution.
*/

#include "../fb.h"

FILE *fb_hAmigaPipeOpen(const char *command, const char *mode);

static int pipe_eof(FB_FILE *handle)
{
    FILE *stream;
    int character, result;
    FB_LOCK();
    stream = handle != NULL ? handle->opaque : NULL;
    result = TRUE;
    if (stream != NULL) {
        /* Pipes have neither a file size nor a seek position, including when
           OPEN PIPE uses BINARY mode. Peek without consuming the next byte. */
        character = fgetc(stream);
        result = character == EOF;
        if (!result) ungetc(character, stream);
    }
    FB_UNLOCK();
    return result ? FB_TRUE : FB_FALSE;
}

static FB_FILE_HOOKS pipe_hooks = {
    pipe_eof, fb_DevPipeClose, NULL, NULL,
    fb_DevFileRead, fb_DevFileReadWstr, fb_DevFileWrite, fb_DevFileWriteWstr,
    NULL, NULL, fb_DevFileReadLine, fb_DevFileReadLineWstr, NULL, fb_DevFileFlush
};

int fb_DevPipeOpen(FB_FILE *handle, const char *filename, size_t length)
{
    const char *mode;
    int access;
    if (handle == NULL || filename == NULL || length == 0)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    if (handle->mode == FB_FILE_MODE_INPUT) { mode = "r"; access = FB_FILE_ACCESS_READ; }
    else if (handle->mode == FB_FILE_MODE_OUTPUT) { mode = "w"; access = FB_FILE_ACCESS_WRITE; }
    else if (handle->mode == FB_FILE_MODE_BINARY) {
        access = handle->access == FB_FILE_ACCESS_READ ? FB_FILE_ACCESS_READ : FB_FILE_ACCESS_WRITE;
        mode = access == FB_FILE_ACCESS_READ ? "rb" : "wb";
    } else return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    if (handle->access != FB_FILE_ACCESS_ANY && handle->access != access)
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    FB_LOCK();
    handle->opaque = fb_hAmigaPipeOpen(filename, mode);
    if (handle->opaque != NULL) {
        handle->access = access;
        handle->type = FB_FILE_TYPE_PIPE;
        handle->hooks = &pipe_hooks;
    }
    FB_UNLOCK();
    return fb_ErrorSetNum(handle->opaque != NULL ? FB_RTERROR_OK : FB_RTERROR_FILENOTFOUND);
}

/* end of amiga/dev_pipe_open.c */
