/*
    FreeBASIC gfxlib2 support for AmigaOS
    ----------------------------------

    File: gfx_driver.c

    Purpose:

        Adapt the native AmigaOS display and input services to GFXDRIVER.

    Responsibilities:

        - validate the mode contract
        - coordinate display and input lifecycle
        - present dirty rows when drawing unlocks
        - expose useful windowed mode choices

    This file intentionally does NOT contain:

        - pixel conversion
        - Intuition message translation
        - generic gfxlib2 drawing code
*/

#include "fb_gfx_amiga.h"

#include <stdlib.h>

#define FB_AMIGA_SCREENLIST(width, height) ((height) | ((width) << 16))

static const int amiga_mode_sizes[][2] =
{
    { 320, 200 }, { 320, 240 }, { 640, 480 }, { 800, 600 },
    { 1024, 768 }, { 1280, 720 }, { 1280, 1024 }, { 1920, 1080 }
};

static int amiga_driver_init(char *title, int width, int height, int depth,
    int refresh_rate, int flags)
{
    (void)depth;

    if (flags & (DRIVER_OPENGL | DRIVER_SHAPED_WINDOW | DRIVER_RESIZABLE))
        return -1;
    if (fb_amigaGfxDisplayInit(title, width, height, refresh_rate, flags) != 0)
        return -1;

    fb_amigaGfxInputInit();
    __fb_gfx->refresh_rate = fb_amiga_gfx.refresh_rate;
    return 0;
}

static void amiga_driver_exit(void)
{
    fb_amigaGfxInputExit();
    fb_amigaGfxDisplayExit();
}

static void amiga_driver_lock(void)
{
}

static void amiga_driver_unlock(void)
{
    fb_amigaGfxPresent();
}

static void amiga_driver_set_palette(int index, int red, int green, int blue)
{
    fb_amigaGfxSetPalette(index, red, green, blue);
}

static int *amiga_driver_fetch_modes(int depth, int *size)
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

    count = (int)(sizeof(amiga_mode_sizes) / sizeof(amiga_mode_sizes[0]));
    modes = (int *)malloc((size_t)count * sizeof(int));
    if (modes == NULL)
        return NULL;

    for (index = 0; index < count; ++index)
        modes[index] = FB_AMIGA_SCREENLIST(amiga_mode_sizes[index][0],
            amiga_mode_sizes[index][1]);

    *size = count;
    return modes;
}

const GFXDRIVER fb_gfxDriverAmiga =
{
    "AMIGA",
    amiga_driver_init,
    amiga_driver_exit,
    amiga_driver_lock,
    amiga_driver_unlock,
    amiga_driver_set_palette,
    fb_amigaGfxWaitVSync,
    fb_amigaGfxGetMouse,
    NULL,
    NULL,
    fb_amigaGfxSetMouse,
    fb_amigaGfxSetWindowTitle,
    fb_amigaGfxSetWindowPosition,
    amiga_driver_fetch_modes,
    NULL,
    fb_amigaGfxPollEvents,
    fb_amigaGfxPresent,
    NULL
};

/* end of gfx_driver.c */
