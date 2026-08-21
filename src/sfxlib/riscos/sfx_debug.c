/*
    FreeBASIC Sound Library support for RISC OS
    -------------------------------------------

    File: sfx_debug.c

    Purpose:

        Provide a file-only diagnostic channel for sfxlib on RISC OS.

    Responsibilities:

        - read SFXLIB_DEBUG and SFXLIB_DEBUG_LOG once per process
        - expose the shared sfxlib diagnostic interface
        - prefix, write, and flush formatted diagnostics to the selected file

    This file intentionally does NOT contain:

        - terminal or VDU output
        - audio driver state
        - subsystem initialization or recovery

    Sound initialization can occur while gfxlib2 owns the physical
    framebuffer. Standard-error output would pass through the RISC OS VDU and
    damage that display, so this platform replacement never falls back to it.
*/

#include "../fb_sfx_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------------- */
/* Diagnostic state                                                         */
/* ------------------------------------------------------------------------- */

static int g_riscos_sfx_debug_initialized;
static FILE *g_riscos_sfx_debug_stream;

void fb_sfxDebugInit(void)
{
    const char *enabled;
    const char *path;

    if (g_riscos_sfx_debug_initialized)
        return;

    g_riscos_sfx_debug_initialized = 1;
    enabled = getenv("SFXLIB_DEBUG");
    if (enabled == NULL || enabled[0] == '\0' ||
        (enabled[0] == '0' && enabled[1] == '\0'))
    {
        return;
    }

    path = getenv("SFXLIB_DEBUG_LOG");
    if (path == NULL || path[0] == '\0')
        return;

    g_riscos_sfx_debug_stream = fopen(path, "a");
}

int fb_sfxDebugEnabled(void)
{
    fb_sfxDebugInit();
    return g_riscos_sfx_debug_stream != NULL;
}

/* ------------------------------------------------------------------------- */
/* Logging                                                                   */
/* ------------------------------------------------------------------------- */

void fb_sfxDebugLog(const char *format, ...)
{
    va_list arguments;

    if (!fb_sfxDebugEnabled())
        return;

    fputs("SFX: ", g_riscos_sfx_debug_stream);
    va_start(arguments, format);
    vfprintf(g_riscos_sfx_debug_stream, format, arguments);
    va_end(arguments);
    fputc('\n', g_riscos_sfx_debug_stream);
    fflush(g_riscos_sfx_debug_stream);
}

/* end of sfx_debug.c */
