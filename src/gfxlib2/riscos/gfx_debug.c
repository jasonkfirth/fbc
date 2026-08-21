/*
    FreeBASIC gfxlib2 support for RISC OS
    -------------------------------------

    File: gfx_debug.c

    Purpose:

        Provide one opt-in diagnostic channel for the RISC OS backend.

    Responsibilities:

        - read GFXLIB_DEBUG once per process
        - open the file named by GFXLIB_DEBUG_LOG when diagnostics are enabled
        - prefix, write, and flush formatted backend diagnostics

    This file intentionally does NOT contain:

        - graphics state or implementation logic
        - terminal or VDU output
        - subsystem-specific error recovery

    RISC OS graphics programs write directly to the physical framebuffer.
    Sending diagnostics to stderr would pass text through the VDU and alter
    the active graphics mode. Diagnostics therefore remain disabled unless
    both the enable flag and a file destination are present.
*/

#include "fb_gfx_riscos.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------------- */
/* Diagnostic destination                                                    */
/* ------------------------------------------------------------------------- */

static FILE *riscos_gfx_debug_stream(void)
{
    static int initialized;
    static FILE *stream;
    const char *enabled;
    const char *path;

    if (initialized)
        return stream;

    initialized = 1;
    enabled = getenv("GFXLIB_DEBUG");
    if (enabled == NULL || enabled[0] == '\0' ||
        (enabled[0] == '0' && enabled[1] == '\0'))
    {
        return NULL;
    }

    path = getenv("GFXLIB_DEBUG_LOG");
    if (path == NULL || path[0] == '\0')
        return NULL;

    stream = fopen(path, "a");
    return stream;
}

/* ------------------------------------------------------------------------- */
/* Public logging entry point                                                */
/* ------------------------------------------------------------------------- */

void fb_riscosGfxDebug(const char *format, ...)
{
    va_list arguments;
    FILE *stream;

    stream = riscos_gfx_debug_stream();
    if (stream == NULL)
        return;

    fputs("gfxlib2: RISC OS: ", stream);
    va_start(arguments, format);
    vfprintf(stream, format, arguments);
    va_end(arguments);
    fputc('\n', stream);
    fflush(stream);
}

/* end of gfx_debug.c */
