/*
    FreeBASIC gfxlib2 support for AROS
    ----------------------------------

    File: gfx_driver.c

    Purpose:

        Adapt the native AROS display and input services to GFXDRIVER.

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

#include "fb_gfx_aros.h"

#include <stdlib.h>

#define FB_AROS_SCREENLIST(width, height) ((height) | ((width) << 16))

static const int aros_mode_sizes[][2] =
{
    { 320, 200 }, { 320, 240 }, { 640, 480 }, { 800, 600 },
    { 1024, 768 }, { 1280, 720 }, { 1280, 1024 }, { 1920, 1080 }
};

static int aros_driver_init(char *title, int width, int height, int depth,
    int refresh_rate, int flags)
{
    (void)depth;

    if (flags & (DRIVER_OPENGL | DRIVER_SHAPED_WINDOW | DRIVER_RESIZABLE))
        return -1;
    if (fb_arosGfxDisplayInit(title, width, height, refresh_rate, flags) != 0)
        return -1;

    fb_arosGfxInputInit();
    __fb_gfx->refresh_rate = fb_aros_gfx.refresh_rate;
    return 0;
}

static void aros_driver_exit(void)
{
    fb_arosGfxInputExit();
    fb_arosGfxDisplayExit();
}

static void aros_driver_lock(void)
{
}

static void aros_driver_unlock(void)
{
    fb_arosGfxPresent();
}

static void aros_driver_set_palette(int index, int red, int green, int blue)
{
    (void)index;
    (void)red;
    (void)green;
    (void)blue;
}

static int *aros_driver_fetch_modes(int depth, int *size)
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

    count = (int)(sizeof(aros_mode_sizes) / sizeof(aros_mode_sizes[0]));
    modes = (int *)malloc((size_t)count * sizeof(int));
    if (modes == NULL)
        return NULL;

    for (index = 0; index < count; ++index)
        modes[index] = FB_AROS_SCREENLIST(aros_mode_sizes[index][0],
            aros_mode_sizes[index][1]);

    *size = count;
    return modes;
}

const GFXDRIVER fb_gfxDriverAros =
{
    "AROS",
    aros_driver_init,
    aros_driver_exit,
    aros_driver_lock,
    aros_driver_unlock,
    aros_driver_set_palette,
    fb_arosGfxWaitVSync,
    fb_arosGfxGetMouse,
    NULL,
    NULL,
    fb_arosGfxSetMouse,
    fb_arosGfxSetWindowTitle,
    fb_arosGfxSetWindowPosition,
    aros_driver_fetch_modes,
    NULL,
    fb_arosGfxPollEvents,
    fb_arosGfxPresent,
    NULL
};

/* end of gfx_driver.c */
