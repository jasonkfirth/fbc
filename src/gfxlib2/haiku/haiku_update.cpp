
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
/* Copy helpers                                                              */
/* ------------------------------------------------------------------------- */

static void fb_hHaikuCopy32To32(
    const uint8_t *src,
    int src_pitch,
    uint8_t *dst,
    int dst_pitch,
    int width,
    int height)
{
    int row_bytes = width * 4;

    for (int y = 0; y < height; y++)
    {
        memcpy(dst, src, row_bytes);
        src += src_pitch;
        dst += dst_pitch;
    }
}

/* ------------------------------------------------------------------------- */
/* Framebuffer copy                                                          */
/* ------------------------------------------------------------------------- */

static void fb_hHaikuCopyFramebuffer(void)
{
    if (!__fb_gfx || !g_bmp)
        return;

    uint8_t *src = (uint8_t*)__fb_gfx->framebuffer;
    uint8_t *dst = (uint8_t*)g_bmp->Bits();

    if (!src || !dst)
        return;

    int src_pitch = __fb_gfx->pitch;
    int dst_pitch = g_bmp->BytesPerRow();

    int w = __fb_gfx->w;
    int h = __fb_gfx->h;

    /* For now assume 32-bit (your test case uses 32bpp) */
    fb_hHaikuCopy32To32(src, src_pitch, dst, dst_pitch, w, h);
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
