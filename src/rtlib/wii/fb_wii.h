/*
    FreeBASIC runtime Wii support
    -----------------------------

    File: fb_wii.h

    Purpose:

        Define the target-local runtime surface for Nintendo Wii builds.

    Responsibilities:

        - include libogc/newlib headers needed by runtime code
        - define FreeBASIC calling and newline conventions
        - expose shared Wii video initialization helpers

    This file intentionally does NOT contain:

        - graphics framebuffer presentation
        - sound playback
        - controller polling
*/

#ifndef FB_WII_H
#define FB_WII_H

#include <errno.h>
#include <gccore.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

#define FBCALL

/* newline for console/file I/O */
#define FB_NEWLINE "\n"
#define FB_NEWLINE_WSTR _LC("\n")

/* newline for printer I/O */
#define FB_BINARY_NEWLINE "\n"
#define FB_BINARY_NEWLINE_WSTR _LC("\n")

#define FB_LL_FMTMOD "ll"
#define FB_CONSOLE_MAXPAGES 1
#define FB_DYLIB void*

typedef off_t fb_off_t;

#ifndef alloca
#define alloca(size) __builtin_alloca(size)
#endif

void fb_WiiVideoInit(void);
GXRModeObj *fb_WiiGetRenderMode(void);
void *fb_WiiGetConsoleFrameBuffer(void);

#endif

/* end of fb_wii.h */
