/*
    FreeBASIC sound library for classic AmigaOS
    ------------------------------------------

    File: sfx_debug.c

    Purpose:
        Route the shared sound diagnostic interface through AmigaDOS.

    Responsibilities:
        - read the native SFXLIB_DEBUG environment variable
        - bound formatted messages and write them to the current output

    This file intentionally does NOT contain:
        - driver state, mixer policy, or unconditional diagnostic output

    Initialization runs before the audio worker is created. Native GetVar
    reads ENV: on a ROM boot, where the SDK's C environment may be empty.
    DOS output is also independent of the parent's buffered C streams.
*/

#include "../fb_sfx_internal.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <stdarg.h>
#include <stdio.h>

static int debug_initialized;
static int debug_enabled;
static BPTR debug_output;
static struct SignalSemaphore debug_lock;

void fb_sfxDebugInit(void)
{
    char setting[4];
    LONG length;

    if (debug_initialized) return;
    length = GetVar("SFXLIB_DEBUG", setting, sizeof(setting), 0);
    debug_enabled = length > 0 && setting[0] != '0';
    /* pthread processes do not inherit the command's Output() stream. Borrow
       it while the sound subsystem is alive; shutdown joins every worker
       before the command releases this handle. Serialize complete messages. */
    debug_output = Output();
    InitSemaphore(&debug_lock);
    debug_initialized = 1;
}

int fb_sfxDebugEnabled(void)
{
    fb_sfxDebugInit();
    return debug_enabled;
}

void fb_sfxDebugLog(const char *format, ...)
{
    char message[256];
    va_list arguments;
    int length;
    BPTR output;

    if (!fb_sfxDebugEnabled() || format == NULL) return;
    va_start(arguments, format);
    length = vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    if (length < 0) return;
    if ((size_t)length >= sizeof(message)) length = sizeof(message) - 1;
    output = debug_output;
    if (output == 0) return;
    ObtainSemaphore(&debug_lock);
    Write(output, "SFX: ", 5);
    Write(output, message, length);
    Write(output, "\n", 1);
    ReleaseSemaphore(&debug_lock);
}

/* end of sfx_debug.c */
