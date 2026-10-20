/*
    FreeBASIC gfxlib2 support for RISC OS
    -------------------------------------

    File: gfx_driver.c

    Purpose:

        Adapt the native RISC OS display and input helpers to GFXDRIVER.

    Responsibilities:

        - validate gfxlib2 mode flags
        - coordinate display and input lifecycle
        - present dirty framebuffer rows when drawing unlocks
        - expose common mode choices to SCREENLIST

    This file intentionally does NOT contain:

        - screen-memory pixel conversion
        - RISC OS keyboard tables
        - generic gfxlib2 drawing code
*/

#include "fb_gfx_riscos.h"

#include <stdlib.h>

#define FB_RISCOS_SCREENLIST(width, height) \
    ((height) | ((width) << 16))

static const int riscos_mode_sizes[][2] =
{
    { 320, 200 },
    { 320, 240 },
    { 640, 480 },
    { 800, 600 },
    { 1024, 768 }
};

static int riscos_driver_init(char *title, int width, int height, int depth,
    int refresh_rate, int flags)
{
    (void)depth;

    if (flags & (DRIVER_OPENGL | DRIVER_SHAPED_WINDOW | DRIVER_RESIZABLE))
        return -1;

    if (fb_riscosGfxDisplayInit(title, width, height, refresh_rate,
        flags) != 0)
        return -1;

    fb_riscosGfxInputInit();
    __fb_gfx->refresh_rate = (refresh_rate > 0) ? refresh_rate : 60;

    return 0;
}

static void riscos_driver_exit(void)
{
    fb_riscosGfxInputExit();
    fb_riscosGfxDisplayExit();
}

static void riscos_driver_lock(void)
{
    /* All screen writes occur synchronously while gfxlib2 holds its lock. */
}

static void riscos_driver_unlock(void)
{
    fb_riscosGfxPresent();
}

static void riscos_driver_set_palette(int index, int red, int green, int blue)
{
    (void)index;
    (void)red;
    (void)green;
    (void)blue;

    /*
        gfx_palette.c already maintains device_palette.  Presentation reads
        that canonical table, so no hardware palette mutation is required.
    */
}

static int *riscos_driver_fetch_modes(int depth, int *size)
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

    count = (int)(sizeof(riscos_mode_sizes) / sizeof(riscos_mode_sizes[0]));
    modes = (int *)malloc((size_t)count * sizeof(int));
    if (modes == NULL)
        return NULL;

    for (index = 0; index < count; ++index)
    {
        modes[index] = FB_RISCOS_SCREENLIST(riscos_mode_sizes[index][0],
            riscos_mode_sizes[index][1]);
    }

    *size = count;
    return modes;
}

const GFXDRIVER fb_gfxDriverRiscos =
{
    "RISC OS",
    riscos_driver_init,
    riscos_driver_exit,
    riscos_driver_lock,
    riscos_driver_unlock,
    riscos_driver_set_palette,
    fb_riscosGfxWaitVSync,
    fb_riscosGfxGetMouse,
    NULL,
    NULL,
    fb_riscosGfxSetMouse,
    NULL,
    NULL,
    riscos_driver_fetch_modes,
    NULL,
    fb_riscosGfxPollEvents,
    fb_riscosGfxPresent,
    NULL
};

/* end of gfx_driver.c */
