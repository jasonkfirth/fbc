/*
    FreeBASIC gfxlib2 support for Windows CE
    ----------------------------------------

    File: gfx_driver.c

    Purpose:

        Adapt the native Windows CE display and input services to GFXDRIVER.

    Responsibilities:

        - validate the Windows CE mode contract
        - coordinate display and input lifecycle
        - register the native driver and null fallback
        - expose screen information and native handles

    This file intentionally does NOT contain:

        - pixel conversion
        - window-message translation
        - generic gfxlib2 drawing code
*/

#include "fb_gfx_wince.h"

#include <stdlib.h>

#define FB_WINCE_SCREENLIST(width, height) ((height) | ((width) << 16))

static const int wince_mode_sizes[][2] =
{
    { 240, 320 }, { 320, 240 }, { 320, 480 }, { 480, 640 },
    { 640, 480 }, { 800, 480 }, { 800, 600 }, { 1024, 600 },
    { 1024, 768 }
};

static int wince_driver_init(char *title, int width, int height, int depth,
    int refresh_rate, int flags)
{
    (void)depth;

    if (flags & (DRIVER_OPENGL | DRIVER_SHAPED_WINDOW | DRIVER_RESIZABLE))
        return -1;
    if (fb_winceGfxDisplayInit(title, width, height,
        refresh_rate, flags) != 0)
    {
        return -1;
    }

    fb_winceGfxInputInit();
    __fb_gfx->refresh_rate = fb_wince_gfx.refresh_rate;
    return 0;
}

static void wince_driver_exit(void)
{
    fb_winceGfxInputExit();
    fb_winceGfxDisplayExit();
}

static void wince_driver_lock(void)
{
}

static void wince_driver_unlock(void)
{
    fb_winceGfxPollEvents();
    fb_winceGfxPresent();
}

static void wince_driver_set_palette(int index, int red, int green, int blue)
{
    (void)index;
    (void)red;
    (void)green;
    (void)blue;

    /* The display converter reads gfxlib2's canonical device palette. */
}

static int *wince_driver_fetch_modes(int depth, int *size)
{
    int *modes;
    int count;
    int index;

    if (size == NULL)
        return NULL;
    *size = 0;

    if (depth != 0 && depth != 1 && depth != 2 && depth != 4 &&
        depth != 8 && depth != 15 && depth != 16 && depth != 24 &&
        depth != 32)
    {
        return NULL;
    }

    count = (int)(sizeof(wince_mode_sizes) / sizeof(wince_mode_sizes[0]));
    modes = (int *)malloc((size_t)count * sizeof(int));
    if (modes == NULL)
        return NULL;

    for (index = 0; index < count; ++index)
    {
        modes[index] = FB_WINCE_SCREENLIST(wince_mode_sizes[index][0],
            wince_mode_sizes[index][1]);
    }

    *size = count;
    return modes;
}

const GFXDRIVER fb_gfxDriverWinCE =
{
    "Windows CE GDI",
    wince_driver_init,
    wince_driver_exit,
    wince_driver_lock,
    wince_driver_unlock,
    wince_driver_set_palette,
    fb_winceGfxWaitVSync,
    fb_winceGfxGetMouse,
    NULL,
    NULL,
    fb_winceGfxSetMouse,
    fb_winceGfxSetWindowTitle,
    fb_winceGfxSetWindowPosition,
    wince_driver_fetch_modes,
    NULL,
    fb_winceGfxPollEvents,
    fb_winceGfxPresent,
    NULL
};

const GFXDRIVER *__fb_gfx_drivers_list[] =
{
    &fb_gfxDriverWinCE,
    &__fb_gfxDriverNull,
    NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth,
    ssize_t *refresh)
{
    fb_winceGfxReadScreenInfo(width, height, depth, refresh);
}

ssize_t fb_hGetWindowHandle(void)
{
    return (ssize_t)fb_wince_gfx.window;
}

ssize_t fb_hGetDisplayHandle(void)
{
    return (ssize_t)fb_wince_gfx.window;
}

/* end of gfx_driver.c */
