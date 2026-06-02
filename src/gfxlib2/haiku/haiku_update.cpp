
#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_window.h"

#include <Bitmap.h>
#include <View.h>
#include <Window.h>
#include <Screen.h>

#include <OS.h>

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------------- */

extern BBitmap *g_bmp;
extern BView   *g_view;

#define FB_HAIKU_REDRAW_MSG 'fbrd'

/* ------------------------------------------------------------------------- */
/* Framebuffer copy                                                          */
/* ------------------------------------------------------------------------- */

static void fb_hHaikuCopyFramebuffer(void)
{
    if (!__fb_gfx || !g_bmp)
        return;

    uint8_t *dst = (uint8_t*)g_bmp->Bits();
    BLITTER *blitter;

    if (!dst)
        return;

    blitter = fb_hGetBlitter(32, FALSE);

    if (!blitter)
        return;

    /*
        Haiku presents through a 32-bit BBitmap, while the gfxlib framebuffer
        can still be an old 1, 2, 4, or 8-bit SCREEN mode.  Use the shared
        blitter so palette modes are expanded safely instead of treating every
        source row as 32-bit pixels.
    */
    blitter(dst, g_bmp->BytesPerRow());
}

/* ------------------------------------------------------------------------- */
/* Frame presentation                                                        */
/* ------------------------------------------------------------------------- */

void fb_hHaikuUpdate(void)
{
    if (!__fb_gfx || !g_view || !g_bmp)
        return;

    fb_hHaikuCopyFramebuffer();

    BWindow *win = g_view->Window();

    if (win)
    {
        /* Safe async trigger of redraw on GUI thread */
        win->PostMessage(FB_HAIKU_REDRAW_MSG, g_view);
    }

    /* clear dirty flags if present */
    if (__fb_gfx->dirty)
    {
        memset(
            __fb_gfx->dirty,
            0,
            __fb_gfx->h * __fb_gfx->scanline_size
        );
    }
}

/* ------------------------------------------------------------------------- */
/* Event polling                                                             */
/* ------------------------------------------------------------------------- */

void fb_hHaikuPollEvents(void)
{
    /* Do NOT drive rendering from here anymore */

    /* Yield CPU (Sleep 0 equivalent) */
    snooze(0);
}

/* ------------------------------------------------------------------------- */
/* Palette                                                                   */
/* ------------------------------------------------------------------------- */

void fb_hHaikuSetPalette(int index, int r, int g, int b)
{
    if (!__fb_gfx)
        return;

    if (index < 0 || index >= 256)
        return;

    __fb_gfx->palette[index] =
        ((r & 255) << 16) |
        ((g & 255) << 8) |
        (b & 255);
}

/* ------------------------------------------------------------------------- */
/* Vertical sync                                                             */
/* ------------------------------------------------------------------------- */

void fb_hHaikuWaitVSync(void)
{
    BScreen screen;
    screen.WaitForRetrace();
}

#endif
