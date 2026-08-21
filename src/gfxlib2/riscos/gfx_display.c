/*
    FreeBASIC gfxlib2 support for RISC OS
    -------------------------------------

    File: gfx_display.c

    Purpose:

        Present gfxlib2's software framebuffer on a native RISC OS display.

    Responsibilities:

        - select and restore RISC OS screen modes
        - discover screen memory through documented VDU variables
        - protect native cursors while writing directly to screen memory
        - convert paletted and true-colour FreeBASIC pixels to RISC OS pixels
        - copy dirty scanlines and clean cached screen memory

    This file intentionally does NOT contain:

        - keyboard or mouse translation
        - generic drawing primitives
        - Wimp window management

    Display model:

        RISC OS 3.5 and newer define 16bpp pixels as 0BBBBBGG GGGRRRRR and
        32bpp pixels as 0x00BBGGRR.  The byte order therefore differs from
        gfxlib2's 0x00RRGGBB true-colour values.  Conversion is explicit so
        the backend behaves identically on VIDC, RPCEmu, and modern cached
        framebuffer drivers.
*/

#include "fb_gfx_riscos.h"

#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <swis.h>

/* ------------------------------------------------------------------------- */
/* RISC OS display constants                                                 */
/* ------------------------------------------------------------------------- */

#define FB_RISCOS_MODE_FLAG_SELECTOR 1
#define FB_RISCOS_LOG2BPP_16         4
#define FB_RISCOS_LOG2BPP_32         5

#define FB_RISCOS_VDU_LINE_LENGTH    6
#define FB_RISCOS_VDU_SCREEN_SIZE    7
#define FB_RISCOS_VDU_LOG2_BPP       9
#define FB_RISCOS_VDU_X_LIMIT        11
#define FB_RISCOS_VDU_Y_LIMIT        12
#define FB_RISCOS_VDU_X_EIGEN        4
#define FB_RISCOS_VDU_Y_EIGEN        5
#define FB_RISCOS_VDU_SCREEN_START   148

#define FB_RISCOS_SCREENMODE_SELECT      0
#define FB_RISCOS_SCREENMODE_READ        1
#define FB_RISCOS_SCREENMODE_CACHE_CLEAN 5

static const int riscos_physical_modes[][2] =
{
    { 320, 200 },
    { 320, 240 },
    { 640, 480 },
    { 800, 600 },
    { 1024, 768 },
    { 1280, 1024 },
    { 1920, 1080 }
};

FB_RISCOS_GFX_STATE fb_riscos_gfx;

/* ------------------------------------------------------------------------- */
/* SWI helpers                                                               */
/* ------------------------------------------------------------------------- */

static int riscos_screen_mode(int reason, int mode)
{
    _kernel_oserror *error;
    _kernel_swi_regs registers;

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = reason;
    registers.r[1] = mode;

    error = _kernel_swi(OS_ScreenMode, &registers, &registers);
    if (error != NULL)
    {
        fb_riscosGfxDebug("OS_ScreenMode %d failed: %s", reason,
            error->errmess);
        return -1;
    }

    fb_riscosGfxDebug("OS_ScreenMode %d returned mode 0x%08X", reason,
        (unsigned int)registers.r[1]);
    return registers.r[1];
}

int fb_riscosGfxReadVduVariables(void)
{
    static const int variables[] =
    {
        FB_RISCOS_VDU_SCREEN_START,
        FB_RISCOS_VDU_LINE_LENGTH,
        FB_RISCOS_VDU_SCREEN_SIZE,
        FB_RISCOS_VDU_LOG2_BPP,
        FB_RISCOS_VDU_X_LIMIT,
        FB_RISCOS_VDU_Y_LIMIT,
        FB_RISCOS_VDU_X_EIGEN,
        FB_RISCOS_VDU_Y_EIGEN,
        -1
    };
    int values[8];
    _kernel_swi_regs registers;

    memset(values, 0, sizeof(values));
    memset(&registers, 0, sizeof(registers));
    registers.r[0] = (int)variables;
    registers.r[1] = (int)values;

    if (_kernel_swi(OS_ReadVduVariables, &registers, &registers) != NULL)
        return -1;

    fb_riscos_gfx.screen = (unsigned char *)values[0];
    fb_riscos_gfx.screen_pitch = values[1];
    fb_riscos_gfx.screen_size = values[2];
    fb_riscos_gfx.screen_depth = 1 << values[3];
    fb_riscos_gfx.screen_width = values[4] + 1;
    fb_riscos_gfx.screen_height = values[5] + 1;
    fb_riscos_gfx.x_eigen = values[6];
    fb_riscos_gfx.y_eigen = values[7];

    fb_riscosGfxDebug("screen=%p pitch=%d size=%d mode=%dx%dx%d eigen=%d,%d",
        (void *)fb_riscos_gfx.screen, fb_riscos_gfx.screen_pitch,
        fb_riscos_gfx.screen_size, fb_riscos_gfx.screen_width,
        fb_riscos_gfx.screen_height, fb_riscos_gfx.screen_depth,
        fb_riscos_gfx.x_eigen, fb_riscos_gfx.y_eigen);

    if (fb_riscos_gfx.screen == NULL ||
        fb_riscos_gfx.screen_pitch <= 0 ||
        fb_riscos_gfx.screen_size <= 0 ||
        fb_riscos_gfx.screen_width <= 0 ||
        fb_riscos_gfx.screen_height <= 0)
    {
        return -1;
    }

    if (fb_riscos_gfx.screen_depth != 8 &&
        fb_riscos_gfx.screen_depth != 16 &&
        fb_riscos_gfx.screen_depth != 32)
    {
        return -1;
    }

    return 0;
}

static int riscos_remove_cursors(void)
{
    _kernel_oserror *error;
    _kernel_swi_regs registers;

    if (fb_riscos_gfx.cursors_removed)
        return 0;

    memset(&registers, 0, sizeof(registers));
    error = _kernel_swi(OS_RemoveCursors, &registers, &registers);
    if (error != NULL)
    {
        fb_riscosGfxDebug("OS_RemoveCursors failed: %s", error->errmess);
        return -1;
    }

    fb_riscos_gfx.cursors_removed = 1;
    return 0;
}

static void riscos_restore_cursors(void)
{
    _kernel_oserror *error;
    _kernel_swi_regs registers;

    if (!fb_riscos_gfx.cursors_removed)
        return;

    memset(&registers, 0, sizeof(registers));
    error = _kernel_swi(OS_RestoreCursors, &registers, &registers);
    if (error != NULL)
    {
        fb_riscosGfxDebug("OS_RestoreCursors failed: %s", error->errmess);
        return;
    }

    fb_riscos_gfx.cursors_removed = 0;
}

/* ------------------------------------------------------------------------- */
/* Mode ownership                                                            */
/* ------------------------------------------------------------------------- */

static int riscos_save_current_mode(void)
{
    int current_mode;
    int *selector;
    int words;

    current_mode = riscos_screen_mode(FB_RISCOS_SCREENMODE_READ, 0);
    if (current_mode < 0)
        return -1;

    if (current_mode < 256)
    {
        fb_riscos_gfx.original_mode[0] = current_mode;
        fb_riscos_gfx.original_mode_words = 1;
        return 0;
    }

    selector = (int *)current_mode;

    /*
        A mode selector has five header words, zero or more variable/value
        pairs, and a -1 terminator.  Copy it because OS-owned selector memory
        need not survive the mode change that follows.
    */

    for (words = 0; words < FB_RISCOS_MODE_SPEC_WORDS; ++words)
    {
        fb_riscos_gfx.original_mode[words] = selector[words];

        if (words >= 5 && selector[words] == -1)
        {
            fb_riscos_gfx.original_mode_words = words + 1;
            return 0;
        }
    }

    fb_riscos_gfx.original_mode_words = 0;
    return -1;
}

static int riscos_select_mode_spec(int width, int height, int log2_bpp,
    int refresh_rate)
{
    int selector[6];

    selector[0] = FB_RISCOS_MODE_FLAG_SELECTOR;
    selector[1] = width;
    selector[2] = height;
    selector[3] = log2_bpp;
    selector[4] = (refresh_rate > 0) ? refresh_rate : -1;
    selector[5] = -1;

    if (riscos_screen_mode(FB_RISCOS_SCREENMODE_SELECT,
        (int)selector) < 0)
    {
        return -1;
    }

    fb_riscos_gfx.mode_changed = 1;
    return 0;
}

static int riscos_select_mode(int width, int height, int refresh_rate)
{
    static const int depths[] =
    {
        FB_RISCOS_LOG2BPP_32,
        FB_RISCOS_LOG2BPP_16
    };
    int depth_index;
    int mode_count;
    int mode_index;

    /* Prefer the requested dimensions, then the smallest larger container. */

    for (depth_index = 0;
        depth_index < (int)(sizeof(depths) / sizeof(depths[0]));
        ++depth_index)
    {
        if (riscos_select_mode_spec(width, height, depths[depth_index],
            refresh_rate) == 0)
        {
            return 0;
        }

        mode_count = (int)(sizeof(riscos_physical_modes) /
            sizeof(riscos_physical_modes[0]));

        for (mode_index = 0; mode_index < mode_count; ++mode_index)
        {
            if (riscos_physical_modes[mode_index][0] < width ||
                riscos_physical_modes[mode_index][1] < height ||
                (riscos_physical_modes[mode_index][0] == width &&
                    riscos_physical_modes[mode_index][1] == height))
            {
                continue;
            }

            if (riscos_select_mode_spec(
                riscos_physical_modes[mode_index][0],
                riscos_physical_modes[mode_index][1],
                depths[depth_index], refresh_rate) == 0)
            {
                return 0;
            }
        }
    }

    return -1;
}

static void riscos_restore_mode(void)
{
    int mode;

    if (!fb_riscos_gfx.mode_changed ||
        fb_riscos_gfx.original_mode_words <= 0)
    {
        return;
    }

    mode = (fb_riscos_gfx.original_mode_words == 1)
        ? fb_riscos_gfx.original_mode[0]
        : (int)fb_riscos_gfx.original_mode;

    (void)riscos_screen_mode(FB_RISCOS_SCREENMODE_SELECT, mode);
}

/* ------------------------------------------------------------------------- */
/* Pixel conversion                                                          */
/* ------------------------------------------------------------------------- */

unsigned int fb_riscosGfxSourceRgb(int x, const unsigned char *source)
{
    unsigned int pixel;
    unsigned int red;
    unsigned int green;
    unsigned int blue;

    if (__fb_gfx->depth <= 8)
        return __fb_gfx->device_palette[source[x]];

    if (__fb_gfx->depth == 16)
    {
        pixel = ((const unsigned short *)source)[x];
        red = ((pixel >> 11) & 31U) * 255U / 31U;
        green = ((pixel >> 5) & 63U) * 255U / 63U;
        blue = (pixel & 31U) * 255U / 31U;
        return red | (green << 8) | (blue << 16);
    }

    pixel = ((const unsigned int *)source)[x];
    red = (pixel >> 16) & 255U;
    green = (pixel >> 8) & 255U;
    blue = pixel & 255U;

    return red | (green << 8) | (blue << 16);
}

unsigned int fb_riscosGfxReadSourcePixel(int x, int y)
{
    const unsigned char *source;
    unsigned int rgb;

    /*
        Keep this probe separate from the presentation readback below.  The
        RISC OS acceptance fixture uses it to identify whether a colour
        fault was introduced by gfxlib2 drawing or by native conversion into
        a Wimp sprite or the direct display.
    */

    if (!fb_riscos_gfx.active || __fb_gfx == NULL ||
        __fb_gfx->framebuffer == NULL || x < 0 || y < 0 ||
        x >= fb_riscos_gfx.viewport_width ||
        y >= fb_riscos_gfx.viewport_height)
    {
        return 0;
    }

    source = __fb_gfx->framebuffer + (y * __fb_gfx->pitch);
    rgb = fb_riscosGfxSourceRgb(x, source);
    return 0xFF000000U | ((rgb & 255U) << 16) |
        (rgb & 0x0000FF00U) | ((rgb >> 16) & 255U);
}

static void riscos_convert_line_32(unsigned char *destination,
    const unsigned char *source, int width)
{
    unsigned int *output;
    int x;

    output = (unsigned int *)destination;

    for (x = 0; x < width; ++x)
        output[x] = fb_riscosGfxSourceRgb(x, source);
}

static void riscos_convert_line_16(unsigned char *destination,
    const unsigned char *source, int width)
{
    unsigned short *output;
    unsigned int rgb;
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    int x;

    output = (unsigned short *)destination;

    for (x = 0; x < width; ++x)
    {
        rgb = fb_riscosGfxSourceRgb(x, source);
        red = rgb & 255U;
        green = (rgb >> 8) & 255U;
        blue = (rgb >> 16) & 255U;
        output[x] = (unsigned short)((red >> 3) |
            ((green >> 3) << 5) | ((blue >> 3) << 10));
    }
}

/* ------------------------------------------------------------------------- */
/* Public display lifecycle                                                  */
/* ------------------------------------------------------------------------- */

int fb_riscosGfxDisplayInit(const char *title, int width, int height,
    int refresh_rate, int flags)
{
    memset(&fb_riscos_gfx, 0, sizeof(fb_riscos_gfx));
    fb_riscos_gfx.wimp_task_handle = -1;
    fb_riscos_gfx.wimp_window_handle = -1;

    if ((flags & DRIVER_FULLSCREEN) == 0)
        return fb_riscosGfxWindowInit(title, width, height);

    fb_riscosGfxDebug("initializing fullscreen %dx%d at %d Hz", width,
        height, refresh_rate);

    if (width <= 0 || height <= 0)
        return -1;

    if (riscos_save_current_mode() != 0)
        return -1;

    fb_riscosGfxDebug("saved current mode in %d word(s)",
        fb_riscos_gfx.original_mode_words);

    if (riscos_select_mode(width, height, refresh_rate) != 0)
    {
        /*
            Some monitor definitions reject a small exact mode even though
            their current true-colour mode can contain it.  Retaining that
            mode and centring the logical framebuffer is a safe fallback.
        */

        if (fb_riscosGfxReadVduVariables() != 0 ||
            fb_riscos_gfx.screen_width < width ||
            fb_riscos_gfx.screen_height < height)
        {
            return -1;
        }
    }
    else if (fb_riscosGfxReadVduVariables() != 0 ||
        fb_riscos_gfx.screen_width < width ||
        fb_riscos_gfx.screen_height < height)
    {
        riscos_restore_mode();
        return -1;
    }

    /*
        The direct renderer writes screen memory itself, so it has no palette
        translation step.  An 8bpp desktop is valid for Wimp windows but is
        deliberately not a fullscreen direct-framebuffer target.
    */

    if (fb_riscos_gfx.screen_depth != 16 &&
        fb_riscos_gfx.screen_depth != 32)
    {
        riscos_restore_mode();
        return -1;
    }

    fb_riscos_gfx.viewport_width = width;
    fb_riscos_gfx.viewport_height = height;
    fb_riscos_gfx.viewport_x = (fb_riscos_gfx.screen_width - width) / 2;
    fb_riscos_gfx.viewport_y = (fb_riscos_gfx.screen_height - height) / 2;

    fb_riscosGfxDebug("clearing %d bytes of screen memory",
        fb_riscos_gfx.screen_size);

    /* Keep the cursor manager's saved background out of the initial clear. */

    (void)riscos_remove_cursors();
    memset(fb_riscos_gfx.screen, 0, (size_t)fb_riscos_gfx.screen_size);
    fb_riscos_gfx.active = 1;
    fb_riscosGfxDebug("presenting the initial framebuffer");
    fb_riscosGfxPresent();
    fb_riscosGfxDebug("display initialization complete");

    return 0;
}

void fb_riscosGfxDisplayExit(void)
{
    if (fb_riscos_gfx.windowed)
    {
        fb_riscosGfxWindowExit();
        memset(&fb_riscos_gfx, 0, sizeof(fb_riscos_gfx));
        return;
    }

    riscos_restore_cursors();
    riscos_restore_mode();
    memset(&fb_riscos_gfx, 0, sizeof(fb_riscos_gfx));
}

void fb_riscosGfxPresent(void)
{
    unsigned char *destination;
    const unsigned char *source;
    _kernel_swi_regs registers;
    int y;

    if (fb_riscos_gfx.windowed)
    {
        fb_riscosGfxWindowPresent();
        return;
    }

    if (!fb_riscos_gfx.active || __fb_gfx == NULL ||
        __fb_gfx->framebuffer == NULL || __fb_gfx->dirty == NULL)
    {
        return;
    }

    /*
        OS_RemoveCursors and OS_RestoreCursors are a balanced protocol for
        direct screen access.  Restoring after the cache clean redraws every
        enabled native cursor over the newly presented framebuffer.
    */

    (void)riscos_remove_cursors();
    source = __fb_gfx->framebuffer;

    for (y = 0; y < fb_riscos_gfx.viewport_height; ++y)
    {
        if (!__fb_gfx->dirty[y])
        {
            source += __fb_gfx->pitch;
            continue;
        }

        destination = fb_riscos_gfx.screen +
            ((fb_riscos_gfx.viewport_y + y) * fb_riscos_gfx.screen_pitch) +
            (fb_riscos_gfx.viewport_x * (fb_riscos_gfx.screen_depth / 8));

        if (fb_riscos_gfx.screen_depth == 32)
        {
            riscos_convert_line_32(destination, source,
                fb_riscos_gfx.viewport_width);
        }
        else
        {
            riscos_convert_line_16(destination, source,
                fb_riscos_gfx.viewport_width);
        }

        __fb_gfx->dirty[y] = FALSE;
        source += __fb_gfx->pitch;
    }

    /* Modern RISC OS video drivers may map screen memory as cacheable. */

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = FB_RISCOS_SCREENMODE_CACHE_CLEAN;
    (void)_kernel_swi(OS_ScreenMode, &registers, &registers);
    riscos_restore_cursors();
}

void fb_riscosGfxWaitVSync(void)
{
    (void)_kernel_osbyte(19, 0, 0);
}

unsigned int fb_riscosGfxReadPresentedPixel(int x, int y)
{
    const unsigned char *address;
    unsigned int pixel;
    unsigned int red;
    unsigned int green;
    unsigned int blue;

    /*
        This private probe reads physical screen memory, not gfxlib2's logical
        framebuffer.  RISC OS acceptance tests use it to distinguish a real
        presentation path from a backend that merely accepts drawing calls.
    */

    if (fb_riscos_gfx.windowed)
        return fb_riscosGfxWindowReadPresentedPixel(x, y);

    if (!fb_riscos_gfx.active || x < 0 || y < 0 ||
        x >= fb_riscos_gfx.viewport_width ||
        y >= fb_riscos_gfx.viewport_height)
    {
        return 0;
    }

    address = fb_riscos_gfx.screen +
        ((fb_riscos_gfx.viewport_y + y) * fb_riscos_gfx.screen_pitch) +
        ((fb_riscos_gfx.viewport_x + x) *
            (fb_riscos_gfx.screen_depth / 8));

    if (fb_riscos_gfx.screen_depth == 32)
    {
        pixel = *((const unsigned int *)address);
        red = pixel & 255U;
        green = (pixel >> 8) & 255U;
        blue = (pixel >> 16) & 255U;
    }
    else
    {
        pixel = *((const unsigned short *)address);
        red = ((pixel >> 0) & 31U) * 255U / 31U;
        green = ((pixel >> 5) & 31U) * 255U / 31U;
        blue = ((pixel >> 10) & 31U) * 255U / 31U;
    }

    return 0xFF000000U | (red << 16) | (green << 8) | blue;
}

void fb_riscosGfxReadScreenInfo(ssize_t *width, ssize_t *height,
    ssize_t *depth, ssize_t *refresh)
{
    FB_RISCOS_GFX_STATE saved_state;

    saved_state = fb_riscos_gfx;

    if (fb_riscosGfxReadVduVariables() != 0)
    {
        if (width != NULL)
            *width = 0;
        if (height != NULL)
            *height = 0;
        if (depth != NULL)
            *depth = 0;
        if (refresh != NULL)
            *refresh = 0;
    }
    else
    {
        if (width != NULL)
            *width = fb_riscos_gfx.screen_width;
        if (height != NULL)
            *height = fb_riscos_gfx.screen_height;
        if (depth != NULL)
            *depth = fb_riscos_gfx.screen_depth;
        if (refresh != NULL)
            *refresh = 0;
    }

    fb_riscos_gfx = saved_state;
}

/* end of gfx_display.c */
