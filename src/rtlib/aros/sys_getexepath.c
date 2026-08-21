/*
    FreeBASIC runtime support for AROS
    ----------------------------------

    File: sys_getexepath.c

    Purpose:

        Return the native directory containing the running program.

    Responsibilities:

        - obtain the process-owned PROGDIR lock from dos.library
        - convert the lock to an AROS DOS path
        - fall back to the saved command name when no program lock exists
        - reject truncated results

    This file intentionally does NOT contain:

        - generic executable-name handling
        - Unix procfs assumptions
        - architecture-specific path policy

    Lock ownership:

        GetProgramDir() returns process-owned state.  The runtime must not
        unlock or replace it.
*/

#include "../fb.h"

#include <proto/dos.h>

static char *aros_pathFromArgv(char *dst, ssize_t maxlen)
{
    const char *name;
    const char *separator;
    const char *cursor;
    size_t length;

    if (__fb_ctx.argc <= 0 || __fb_ctx.argv == NULL ||
        __fb_ctx.argv[0] == NULL)
    {
        return NULL;
    }

    name = __fb_ctx.argv[0];
    separator = NULL;
    for (cursor = name; *cursor != '\0'; ++cursor)
    {
        if (*cursor == '/' || *cursor == ':')
            separator = cursor;
    }

    if (separator == NULL)
        return NULL;

    length = (size_t)(separator - name);
    if (*separator == ':')
        ++length;
    if (length == 0 || length >= (size_t)maxlen)
        return NULL;

    memcpy(dst, name, length);
    dst[length] = '\0';
    return dst;
}

char *fb_hGetExePath(char *dst, ssize_t maxlen)
{
    BPTR program_dir;

    if (dst == NULL || maxlen <= 1)
        return NULL;

    dst[0] = '\0';
    program_dir = GetProgramDir();
    if (program_dir != BNULL &&
        NameFromLock(program_dir, (STRPTR)dst, (LONG)maxlen) != DOSFALSE)
    {
        return dst;
    }

    dst[0] = '\0';
    return aros_pathFromArgv(dst, maxlen);
}

/* end of aros/sys_getexepath.c */
