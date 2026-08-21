/*
    FreeBASIC gfxlib2 support for AROS
    ----------------------------------

    File: gfx_display.c

    Purpose:

        Present the FreeBASIC software framebuffer in a native AROS window.

    Responsibilities:

        - own the Intuition window and public-screen lock
        - convert every supported gfxlib2 pixel format to byte-order-neutral
          ARGB rows
        - submit dirty row runs through CyberGraphX WritePixelArray()
        - report display handles and desktop geometry

    This file intentionally does NOT contain:

        - raw keyboard translation
        - generic drawing operations
        - direct access to AROS display memory

    Cursor model:

        Presentation targets a normal Intuition RastPort.  The AROS pointer
        remains compositor-owned and is therefore redrawn above every pixel
        update without a task-window transition or cursor save-under race.
*/

#include "fb_gfx_aros.h"

#include <cybergraphx/cybergraphics.h>
#include <graphics/gfx.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>

FB_AROS_GFX_STATE fb_aros_gfx;

static void aros_source_rgb(const unsigned char *source, int x,
    unsigned char *red, unsigned char *green, unsigned char *blue)
{
    unsigned int pixel;

    if (__fb_gfx->depth <= 8)
    {
        pixel = __fb_gfx->device_palette[source[x]];
        *red = (unsigned char)(pixel & 255U);
        *green = (unsigned char)((pixel >> 8) & 255U);
        *blue = (unsigned char)((pixel >> 16) & 255U);
        return;
    }

    if (__fb_gfx->depth == 16)
    {
        pixel = ((const unsigned short *)source)[x];
        *red = (unsigned char)(((pixel >> 11) & 31U) * 255U / 31U);
        *green = (unsigned char)(((pixel >> 5) & 63U) * 255U / 63U);
        *blue = (unsigned char)((pixel & 31U) * 255U / 31U);
        return;
    }

    pixel = ((const unsigned int *)source)[x];
    *red = (unsigned char)((pixel >> 16) & 255U);
    *green = (unsigned char)((pixel >> 8) & 255U);
    *blue = (unsigned char)(pixel & 255U);
}

static void aros_convert_row(int y)
{
    const unsigned char *source;
    unsigned char *destination;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int x;

    source = __fb_gfx->framebuffer + ((size_t)y * __fb_gfx->pitch);
    destination = fb_aros_gfx.present_buffer +
        ((size_t)y * (size_t)fb_aros_gfx.width * 4U);

    for (x = 0; x < fb_aros_gfx.width; ++x)
    {
        aros_source_rgb(source, x, &red, &green, &blue);

        /*
            RECTFMT_ARGB is a sequence of alpha, red, green, and blue bytes.
            AROS compositors are allowed to honour alpha when copying the
            rectangle into the window, so a zero alpha byte makes otherwise
            valid colour data completely transparent.
        */
        destination[0] = 255;
        destination[1] = red;
        destination[2] = green;
        destination[3] = blue;
        destination += 4;
    }
}

static void aros_write_rows(int first_row, int row_count)
{
    unsigned char *source;

    source = fb_aros_gfx.present_buffer +
        ((size_t)first_row * (size_t)fb_aros_gfx.width * 4U);

    (void)WritePixelArray(source, 0, 0,
        (UWORD)(fb_aros_gfx.width * 4),
        fb_aros_gfx.window->RPort,
        (UWORD)fb_aros_gfx.window->BorderLeft,
        (UWORD)(fb_aros_gfx.window->BorderTop + first_row),
        (UWORD)fb_aros_gfx.width, (UWORD)row_count, RECTFMT_ARGB);
}

int fb_arosGfxDisplayInit(const char *title, int width, int height,
    int refresh_rate, int flags)
{
    size_t pixels;

    (void)flags;
    memset(&fb_aros_gfx, 0, sizeof(fb_aros_gfx));

    if (width <= 0 || height <= 0 || width > 16383 || height > 65535)
        return -1;
    if ((size_t)width > (SIZE_MAX / (size_t)height) / 4U)
        return -1;

    fb_aros_gfx.screen = LockPubScreen(NULL);
    if (fb_aros_gfx.screen == NULL)
        return -1;

    fb_aros_gfx.window = OpenWindowTags(NULL,
        WA_CustomScreen, (IPTR)fb_aros_gfx.screen,
        WA_InnerWidth, (IPTR)width,
        WA_InnerHeight, (IPTR)height,
        WA_Title, (IPTR)((title != NULL) ? title : "FreeBASIC"),
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_RMBTrap, TRUE,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY |
            IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_ACTIVEWINDOW |
            IDCMP_INACTIVEWINDOW | IDCMP_REFRESHWINDOW,
        TAG_DONE);

    if (fb_aros_gfx.window == NULL)
    {
        UnlockPubScreen(NULL, fb_aros_gfx.screen);
        fb_aros_gfx.screen = NULL;
        return -1;
    }

    pixels = (size_t)width * (size_t)height;
    fb_aros_gfx.present_buffer_size = pixels * 4U;
    fb_aros_gfx.present_buffer =
        (unsigned char *)malloc(fb_aros_gfx.present_buffer_size);
    if (fb_aros_gfx.present_buffer == NULL)
    {
        fb_arosGfxDisplayExit();
        return -1;
    }

    memset(fb_aros_gfx.present_buffer, 0,
        fb_aros_gfx.present_buffer_size);
    fb_aros_gfx.width = width;
    fb_aros_gfx.height = height;
    fb_aros_gfx.refresh_rate = (refresh_rate > 0) ? refresh_rate : 60;
    fb_aros_gfx.cursor_visible = TRUE;
    fb_aros_gfx.active = TRUE;
    ScreenToFront(fb_aros_gfx.screen);
    WindowToFront(fb_aros_gfx.window);
    ActivateWindow(fb_aros_gfx.window);
    return 0;
}

void fb_arosGfxDisplayExit(void)
{
    fb_aros_gfx.active = FALSE;

    free(fb_aros_gfx.present_buffer);
    fb_aros_gfx.present_buffer = NULL;
    fb_aros_gfx.present_buffer_size = 0;

    if (fb_aros_gfx.window != NULL)
    {
        CloseWindow(fb_aros_gfx.window);
        fb_aros_gfx.window = NULL;
    }

    if (fb_aros_gfx.screen != NULL)
    {
        UnlockPubScreen(NULL, fb_aros_gfx.screen);
        fb_aros_gfx.screen = NULL;
    }
}

void fb_arosGfxPresent(void)
{
    int first_dirty;
    int row;

    if (!fb_aros_gfx.active || fb_aros_gfx.window == NULL ||
        fb_aros_gfx.present_buffer == NULL || __fb_gfx == NULL ||
        __fb_gfx->framebuffer == NULL)
    {
        return;
    }

    first_dirty = -1;
    for (row = 0; row < fb_aros_gfx.height; ++row)
    {
        int dirty;

        dirty = (__fb_gfx->dirty == NULL || __fb_gfx->dirty[row]);
        if (dirty)
        {
            aros_convert_row(row);
            if (__fb_gfx->dirty != NULL)
                __fb_gfx->dirty[row] = FALSE;
            if (first_dirty < 0)
                first_dirty = row;
        }

        if (first_dirty >= 0 && (!dirty || row == fb_aros_gfx.height - 1))
        {
            int last_dirty;

            last_dirty = dirty ? row : row - 1;
            aros_write_rows(first_dirty, last_dirty - first_dirty + 1);
            first_dirty = -1;
        }
    }
}

void fb_arosGfxWaitVSync(void)
{
    WaitTOF();
}

void fb_arosGfxSetWindowTitle(char *title)
{
    if (fb_aros_gfx.window != NULL && title != NULL)
        SetWindowTitles(fb_aros_gfx.window, (CONST_STRPTR)title,
            (CONST_STRPTR)~(IPTR)0);
}

int fb_arosGfxSetWindowPosition(int x, int y)
{
    if (fb_aros_gfx.window == NULL)
        return 0;

    if (x != INT_MIN || y != INT_MIN)
    {
        int target_x;
        int target_y;

        target_x = (x != INT_MIN) ? x : fb_aros_gfx.window->LeftEdge;
        target_y = (y != INT_MIN) ? y : fb_aros_gfx.window->TopEdge;
        MoveWindow(fb_aros_gfx.window,
            target_x - fb_aros_gfx.window->LeftEdge,
            target_y - fb_aros_gfx.window->TopEdge);
    }

    return (fb_aros_gfx.window->LeftEdge & 0xFFFF) |
        (fb_aros_gfx.window->TopEdge << 16);
}

void fb_arosGfxReadScreenInfo(ssize_t *width, ssize_t *height,
    ssize_t *depth, ssize_t *refresh)
{
    struct Screen *screen;

    screen = (fb_aros_gfx.screen != NULL)
        ? fb_aros_gfx.screen
        : LockPubScreen(NULL);

    if (screen == NULL)
    {
        *width = *height = *depth = *refresh = 0;
        return;
    }

    *width = screen->Width;
    *height = screen->Height;
    *depth = GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH);
    *refresh = 0;

    if (screen != fb_aros_gfx.screen)
        UnlockPubScreen(NULL, screen);
}

/* end of gfx_display.c */
