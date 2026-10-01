/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------
    File: amiga/io_isredir.c
    Purpose: Query native console redirection without Unix descriptor assumptions.
    Responsibilities: Check Input/Output handles through dos.library.
    This file intentionally does NOT contain console initialization or I/O.
*/

#include "../fb.h"
#include <proto/dos.h>

int fb_ConsoleIsRedirected(int is_input)
{
    BPTR handle = is_input ? Input() : Output();
    return handle == 0 || !IsInteractive(handle) ? FB_TRUE : FB_FALSE;
}

/* end of amiga/io_isredir.c */
