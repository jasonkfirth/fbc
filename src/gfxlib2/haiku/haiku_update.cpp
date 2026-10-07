/*
    FreeBASIC gfxlib2 Haiku backend
    File: haiku_update.cpp
    Purpose: Convert and present software frames through the native view.
    Responsibilities: Serialize bitmap access with native drawing and resizing.
    This file contains no software page allocation or input event handlers.
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_window.h"

#include <Bitmap.h>
#include <View.h>
#include <Window.h>
#include <Screen.h>

#include <OS.h>

#include <limits.h>
#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------------- */

extern BBitmap *g_bmp;
extern BView   *g_view;

/* ------------------------------------------------------------------------- */
/* Framebuffer copy                                                          */
/* ------------------------------------------------------------------------- */

static bool fb_hHaikuCopyFramebuffer(BRect *damage, int first_line,
    int last_line)
{
    if (!__fb_gfx || !__fb_gfx->framebuffer || !g_bmp || !damage ||
        __fb_gfx->w <= 0 || __fb_gfx->h <= 0 ||
        __fb_gfx->scanline_size <= 0 || !__fb_gfx->dirty)
        return false;

    uint8_t *dst = (uint8_t*)g_bmp->Bits();
    int dst_pitch = g_bmp->BytesPerRow();
    *damage = BRect(0, 0, -1, -1);
    /* All color conversions share the same 32-bit native destination. */
    if (!dst || dst_pitch <= 0 || g_bmp->BitsLength() <= 0 ||
        __fb_gfx->w > INT_MAX / 4 || dst_pitch < __fb_gfx->w * 4 ||
        __fb_gfx->h > INT_MAX / __fb_gfx->scanline_size ||
        g_bmp->Bounds().IntegerWidth() + 1 != __fb_gfx->w ||
        g_bmp->Bounds().IntegerHeight() + 1 !=
            __fb_gfx->h * __fb_gfx->scanline_size ||
        (size_t)(__fb_gfx->h * __fb_gfx->scanline_size) >
            (size_t)g_bmp->BitsLength() / (size_t)dst_pitch ||
        first_line < 0 || last_line < first_line || last_line >= __fb_gfx->h)
        return false;

    if (__fb_gfx->depth == 32)
    {
        /*
            B_RGB32 and gfxlib's 32-bit framebuffer both store BGR bytes.
            Compare marked rows against the last presented bitmap before
            copying. SCREENCOPY can mark an entire page even when only the
            caret changed; this recovers precise damage without changing the
            drawing API or requiring applications to report horizontal bounds.

            The driver mutex and BWindow lock are held throughout. Pitched
            source and destination storage are validated independently because
            native bitmap row alignment need not match gfxlib's alignment.
        */
        if (__fb_gfx->pitch < __fb_gfx->w * 4)
            return false;

        for (int y = first_line; y <= last_line; y++)
        {
            if (!__fb_gfx->dirty[y])
                continue;
            const uint8_t *source = __fb_gfx->framebuffer +
                (size_t)y * __fb_gfx->pitch;
            for (int repeat = 0; repeat < __fb_gfx->scanline_size; repeat++)
            {
                int physical_y = y * __fb_gfx->scanline_size + repeat;
                uint8_t *target = dst + (size_t)physical_y * dst_pitch;
                if (memcmp(source, target, (size_t)__fb_gfx->w * 4) == 0)
                    continue;
                int first_x = 0;
                int last_x = __fb_gfx->w - 1;
                while (first_x < last_x &&
                    memcmp(source + (size_t)first_x * 4,
                        target + (size_t)first_x * 4, 4) == 0)
                    first_x++;
                while (last_x > first_x &&
                    memcmp(source + (size_t)last_x * 4,
                        target + (size_t)last_x * 4, 4) == 0)
                    last_x--;
                memcpy(target + (size_t)first_x * 4,
                    source + (size_t)first_x * 4,
                    (size_t)(last_x - first_x + 1) * 4);
                BRect row_damage(first_x, physical_y, last_x, physical_y);
                *damage = damage->IsValid() ? (*damage | row_damage) : row_damage;
            }
        }
        return true;
    }

    /* Palette and 16-bit modes retain the shared conversion blitter. Palette
       changes can alter every pixel even when source bytes remain identical. */
    BLITTER *blitter = fb_hGetBlitter(32, FALSE);
    if (!blitter)
        return false;
    blitter(dst, dst_pitch);
    *damage = BRect(0, first_line * __fb_gfx->scanline_size,
        __fb_gfx->w - 1, (last_line + 1) * __fb_gfx->scanline_size - 1);
    return true;
}

/* ------------------------------------------------------------------------- */
/* Frame presentation                                                        */
/* ------------------------------------------------------------------------- */

void fb_hHaikuUpdate(void)
{
    if (!__fb_gfx || !g_view || !g_bmp || !__fb_gfx->dirty ||
        __fb_gfx->h <= 0)
        return;

    /* Dirty markers are logical rows. The allocation also leaves space for
       scanline replication, but the shared blitter consumes only h markers. */
    const char *first = (const char*)memchr(__fb_gfx->dirty, TRUE, __fb_gfx->h);
    if (!first)
        return;
    int first_line = (int)(first - __fb_gfx->dirty);
    int last_line = __fb_gfx->h - 1;
    while (last_line > first_line && !__fb_gfx->dirty[last_line])
        last_line--;

    BWindow *win = g_view->Window();
    /* Draw() and bitmap replacement use this same window lock. Failed copies
       or locks leave dirty markers intact so a later unlock can retry. */
    if (!win || !win->Lock())
        return;
    BRect damage;
    if (fb_hHaikuCopyFramebuffer(&damage, first_line, last_line))
    {
        if (damage.IsValid())
        {
            /* Software views own the bitmap. Invalidate coalesces damage
               without posting a message for every drawing primitive. */
            static_cast<FBHaikuView*>(g_view)->InvalidateFramebufferRect(damage);
        }
        memset(__fb_gfx->dirty, 0, __fb_gfx->h);
    }
    win->Unlock();
}

/* ------------------------------------------------------------------------- */
/* Event polling                                                             */
/* ------------------------------------------------------------------------- */

void fb_hHaikuPollEvents(void)
{
    /* BApplication and BWindow already dispatch input on their own threads.
       Polling from GETMOUSE/INKEY must not yield for every query. SLEEP owns
       the caller's scheduling delay, independently of this notification hook. */
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

/* end of haiku_update.cpp */
