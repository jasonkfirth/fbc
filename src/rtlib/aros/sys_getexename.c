/*
    FreeBASIC runtime support for AROS
    ----------------------------------

    File: sys_getexename.c

    Purpose:

        Return the leaf name of the running FreeBASIC program.

    Responsibilities:

        - read the process name saved by fb_Init()
        - recognize the path separators accepted by AROS DOS and POSIX APIs
        - copy a null-terminated name into caller-owned storage

    This file intentionally does NOT contain:

        - executable filesystem discovery
        - allocation of the destination buffer
        - AROS architecture policy
*/

#include "../fb.h"

char *fb_hGetExeName(char *dst, ssize_t maxlen)
{
    const char *name;
    const char *cursor;

    if (dst == NULL || maxlen <= 1)
        return NULL;

    dst[0] = '\0';
    if (__fb_ctx.argc <= 0 || __fb_ctx.argv == NULL ||
        __fb_ctx.argv[0] == NULL)
    {
        return NULL;
    }

    name = __fb_ctx.argv[0];
    for (cursor = name; *cursor != '\0'; ++cursor)
    {
        if (*cursor == '/' || *cursor == ':')
            name = cursor + 1;
    }

    strncpy(dst, name, (size_t)maxlen - 1);
    dst[maxlen - 1] = '\0';
    return dst;
}

/* end of sys_getexename.c */
