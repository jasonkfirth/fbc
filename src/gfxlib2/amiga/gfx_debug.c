/*
    FreeBASIC gfxlib2 support for AmigaOS
    ----------------------------------

    File: gfx_debug.c

    Purpose:

        Centralize opt-in diagnostics for the AMIGA gfxlib2 backend.

    Responsibilities:

        - read the FB_GFX_AMIGA_DEBUG environment switch once
        - keep backend diagnostics away from the graphics display
        - provide one formatted logging entry point

    This file intentionally does NOT contain:

        - unconditional console output
        - display or input operations
        - generic gfxlib2 diagnostics
*/

#include "fb_gfx_amiga.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/dos.h>

void fb_amigaGfxDebug(const char *format, ...)
{
    static int initialized;
    static int enabled;
    va_list arguments;

    if (!initialized)
    {
        char setting[4];
        LONG length = GetVar("FB_GFX_AMIGA_DEBUG", setting, sizeof(setting), 0);
        enabled = length > 0 && setting[0] != '0';
        initialized = TRUE;
    }

    if (!enabled || format == NULL)
        return;

    char message[256];
    LONG length;
    va_start(arguments, format);
    length = vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    if (length < 0) return;
    if ((size_t)length >= sizeof(message)) length = sizeof(message) - 1;
    Write(Output(), "gfxlib2/amiga: ", 15);
    Write(Output(), message, length);
    Write(Output(), "\n", 1);
}

/* end of gfx_debug.c */
