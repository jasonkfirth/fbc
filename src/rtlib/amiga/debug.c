/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/debug.c
    Purpose: Centralize optional native runtime lifecycle diagnostics.
    Responsibilities: Write bounded trace messages without stdio or allocation.
    This file intentionally does NOT contain console or framebuffer operations.
*/

#include "../fb.h"
#include <proto/dos.h>

void fb_hAmigaDebugNumber(const char *message, unsigned long value)
{
    char text[96];
    size_t length = strlen(message);
    int digit;

    if (length > sizeof(text) - 12) return;
    memcpy(text, message, length);
    text[length++] = ' ';
    for (digit = 7; digit >= 0; --digit)
        text[length++] = "0123456789abcdef"[(value >> (digit * 4)) & 15];
    text[length] = '\0';
    fb_hAmigaDebug(text);
}

void fb_hAmigaDebug(const char *message)
{
#ifndef FB_AMIGA_DEBUG
    (void)message;
#else
    BPTR file;
    size_t length;

    if (message == NULL) return;
    length = strlen(message);
    if (length > LONG_MAX - 1) return;
    file = Open("SYS:rtl-trace.txt", MODE_READWRITE);
    if (file == BNULL) return;
    Seek(file, 0, OFFSET_END);
    Write(file, (APTR)message, (LONG)length);
    Write(file, "\n", 1);
    Close(file);
#endif
}

/* end of amiga/debug.c */
