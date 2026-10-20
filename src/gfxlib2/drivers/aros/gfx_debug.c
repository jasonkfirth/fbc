/*
    FreeBASIC gfxlib2 support for AROS
    ----------------------------------

    File: gfx_debug.c

    Purpose:

        Centralize opt-in diagnostics for the AROS gfxlib2 backend.

    Responsibilities:

        - read the FB_GFX_AROS_DEBUG environment switch once
        - keep backend diagnostics away from the graphics display
        - provide one formatted logging entry point

    This file intentionally does NOT contain:

        - unconditional console output
        - display or input operations
        - generic gfxlib2 diagnostics
*/

#include "fb_gfx_aros.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fb_arosGfxDebug(const char *format, ...)
{
    static int initialized;
    static int enabled;
    va_list arguments;

    if (!initialized)
    {
        const char *setting;

        setting = getenv("FB_GFX_AROS_DEBUG");
        enabled = (setting != NULL && setting[0] != '\0' &&
            setting[0] != '0');
        initialized = TRUE;
    }

    if (!enabled || format == NULL)
        return;

    fputs("gfxlib2/aros: ", stderr);
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fputc('\n', stderr);
}

/* end of gfx_debug.c */
