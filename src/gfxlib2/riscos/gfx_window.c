/*
    FreeBASIC gfxlib2 support for RISC OS
    -------------------------------------

    File: gfx_window.c

    Purpose:

        Present the gfxlib2 framebuffer in an ordinary RISC OS Wimp window.

    Responsibilities:

        - register and close the Wimp task owned by a windowed SCREEN mode
        - create a native-format sprite as the Wimp redraw source
        - copy dirty gfxlib2 scanlines into that sprite
        - redraw and update only through Wimp clipping rectangles
        - translate Wimp window, keyboard, and pointer state for this mode

    This file intentionally does NOT contain:

        - direct fullscreen framebuffer access
        - physical RISC OS keyboard scan tables
        - generic gfxlib2 drawing primitives

    Window model:

        The Wimp owns the visible pixels.  gfxlib2 first converts its software
        framebuffer into a user sprite, then plots that sprite while inside a
        Wimp_RedrawWindow or Wimp_UpdateWindow sequence.  This preserves other
        desktop windows and lets the Wimp restore the game after obscuring
        windows move away.
*/

#include "fb_gfx_riscos.h"

#include <kernel.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <swis.h>

/* ------------------------------------------------------------------------- */
/* Wimp and sprite protocol constants                                        */
/* ------------------------------------------------------------------------- */

#define FB_RISCOS_WIMP_VERSION             310
#define FB_RISCOS_WIMP_TASK_MAGIC          0x4B534154

#define FB_RISCOS_WIMP_REDRAW              1
#define FB_RISCOS_WIMP_OPEN                2
#define FB_RISCOS_WIMP_CLOSE               3
#define FB_RISCOS_WIMP_MOUSE_CLICK         6
#define FB_RISCOS_WIMP_KEY                 8
#define FB_RISCOS_WIMP_LOSE_CARET          11
#define FB_RISCOS_WIMP_GAIN_CARET          12
#define FB_RISCOS_WIMP_USER_MESSAGE        17
#define FB_RISCOS_WIMP_USER_MESSAGE_RECORDED 18

#define FB_RISCOS_WIMP_WINDOW_FLAG_CLOSE   (1U << 25)
#define FB_RISCOS_WIMP_WINDOW_FLAG_TITLE   (1U << 26)
#define FB_RISCOS_WIMP_WINDOW_FLAG_NEW     (1U << 31)
#define FB_RISCOS_WIMP_WINDOW_FLAGS \
    (FB_RISCOS_WIMP_WINDOW_FLAG_CLOSE | \
        FB_RISCOS_WIMP_WINDOW_FLAG_TITLE | \
        FB_RISCOS_WIMP_WINDOW_FLAG_NEW)

#define FB_RISCOS_WIMP_TITLE_TEXT          (1U << 0)
#define FB_RISCOS_WIMP_TITLE_CENTRE_X      (1U << 3)
#define FB_RISCOS_WIMP_TITLE_CENTRE_Y      (1U << 4)
#define FB_RISCOS_WIMP_TITLE_FLAGS \
    (FB_RISCOS_WIMP_TITLE_TEXT | FB_RISCOS_WIMP_TITLE_CENTRE_X | \
        FB_RISCOS_WIMP_TITLE_CENTRE_Y)

#define FB_RISCOS_WIMP_WINDOW_FLAG_MOVEABLE (1U << 1)
#define FB_RISCOS_WIMP_WORK_BUTTON_FOCUS   (15U << 12)
#define FB_RISCOS_WIMP_POLL_MASK           ((1U << 4) | (1U << 5))
#define FB_RISCOS_WIMP_INVISIBLE_CARET     (1U << 25)

#define FB_RISCOS_SPRITE_AREA_HEADER_SIZE  16
#define FB_RISCOS_SPRITE_HEADER_SIZE       44
#define FB_RISCOS_SPRITE_PIXEL_OFFSET \
    (FB_RISCOS_SPRITE_AREA_HEADER_SIZE + FB_RISCOS_SPRITE_HEADER_SIZE)
#define FB_RISCOS_SPRITE_INITIALISE        (256 + 9)
#define FB_RISCOS_SPRITE_CREATE            (256 + 15)
#define FB_RISCOS_SPRITE_PLOT              (512 + 52)
#define FB_RISCOS_SPRITE_TRANSLATION_LIMIT (64 * 1024)

#define FB_RISCOS_SCREENMODE_READ          1
#define FB_RISCOS_WINDOW_POLL_LIMIT        32

/* ------------------------------------------------------------------------- */
/* Native block layouts                                                      */
/* ------------------------------------------------------------------------- */

typedef struct FB_RISCOS_SPRITE_AREA
{
    unsigned int size;
    unsigned int count;
    unsigned int start;
    unsigned int end;
} FB_RISCOS_SPRITE_AREA;

typedef union FB_RISCOS_WIMP_BLOCK
{
    int words[64];
    unsigned char bytes[256];
} FB_RISCOS_WIMP_BLOCK;

/* ------------------------------------------------------------------------- */
/* SWI helpers                                                               */
/* ------------------------------------------------------------------------- */

static int riscos_window_swi(int swi, _kernel_swi_regs *registers,
    const char *operation)
{
    _kernel_oserror *error;

    error = _kernel_swi(swi, registers, registers);
    if (error == NULL)
        return 0;

    fb_riscosGfxDebug("%s failed: %s", operation, error->errmess);
    return -1;
}

static int riscos_window_read_mode(void)
{
    _kernel_swi_regs registers;

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = FB_RISCOS_SCREENMODE_READ;

    if (riscos_window_swi(OS_ScreenMode, &registers,
        "OS_ScreenMode read") != 0)
    {
        return -1;
    }

    return registers.r[1];
}

static int riscos_window_read_time(void)
{
    _kernel_swi_regs registers;

    memset(&registers, 0, sizeof(registers));
    if (riscos_window_swi(OS_ReadMonotonicTime, &registers,
        "OS_ReadMonotonicTime") != 0)
    {
        return 0;
    }

    return registers.r[0];
}

/* ------------------------------------------------------------------------- */
/* Wimp window state                                                         */
/* ------------------------------------------------------------------------- */

static void riscos_window_store_state(const FB_RISCOS_WIMP_BLOCK *block)
{
    fb_riscos_gfx.wimp_visible_x0 = block->words[1];
    fb_riscos_gfx.wimp_visible_y1 = block->words[4];
    fb_riscos_gfx.wimp_scroll_x = block->words[5];
    fb_riscos_gfx.wimp_scroll_y = block->words[6];
}

static int riscos_window_open(FB_RISCOS_WIMP_BLOCK *block)
{
    _kernel_swi_regs registers;

    memset(&registers, 0, sizeof(registers));
    registers.r[1] = (int)block;

    if (riscos_window_swi(Wimp_OpenWindow, &registers,
        "Wimp_OpenWindow") != 0)
    {
        return -1;
    }

    riscos_window_store_state(block);
    fb_riscos_gfx.wimp_window_open = 1;
    return 0;
}

static void riscos_window_close(void)
{
    FB_RISCOS_WIMP_BLOCK block;
    _kernel_swi_regs registers;

    if (!fb_riscos_gfx.wimp_window_open ||
        fb_riscos_gfx.wimp_window_handle < 0)
    {
        return;
    }

    memset(&block, 0, sizeof(block));
    memset(&registers, 0, sizeof(registers));
    block.words[0] = fb_riscos_gfx.wimp_window_handle;
    registers.r[1] = (int)&block;

    if (riscos_window_swi(Wimp_CloseWindow, &registers,
        "Wimp_CloseWindow") == 0)
    {
        fb_riscos_gfx.wimp_window_open = 0;
    }
}

static void riscos_window_set_focus(void)
{
    _kernel_swi_regs registers;

    if (!fb_riscos_gfx.wimp_window_open ||
        fb_riscos_gfx.wimp_window_handle < 0)
    {
        return;
    }

    /*
        An invisible work-area caret gives the game keyboard focus without
        drawing a text cursor over its framebuffer.  The Wimp retains normal
        ownership of the physical pointer in windowed mode.
    */

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = fb_riscos_gfx.wimp_window_handle;
    registers.r[1] = -1;
    registers.r[2] = 0;
    registers.r[3] = 0;
    registers.r[4] = FB_RISCOS_WIMP_INVISIBLE_CARET;
    registers.r[5] = 0;
    (void)riscos_window_swi(Wimp_SetCaretPosition, &registers,
        "Wimp_SetCaretPosition");
}

static void riscos_window_post_focus(int focused)
{
    EVENT event;

    if (fb_riscos_gfx.wimp_has_input_focus == focused)
        return;

    fb_riscos_gfx.wimp_has_input_focus = focused;
    memset(&event, 0, sizeof(event));
    event.type = focused ? EVENT_WINDOW_GOT_FOCUS : EVENT_WINDOW_LOST_FOCUS;
    fb_hPostEvent(&event);
}

/* ------------------------------------------------------------------------- */
/* Sprite allocation and conversion                                          */
/* ------------------------------------------------------------------------- */

static int riscos_window_create_sprite(int width, int height)
{
    FB_RISCOS_SPRITE_AREA *area;
    _kernel_swi_regs registers;
    int bytes_per_pixel;
    int mode;
    int pitch;
    int size;
    static const char sprite_name[] = "FreeBASIC";

    bytes_per_pixel = fb_riscos_gfx.screen_depth / 8;
    if (bytes_per_pixel != 1 && bytes_per_pixel != 2 &&
        bytes_per_pixel != 4)
        return -1;

    if (width > (INT_MAX - 3) / bytes_per_pixel)
        return -1;

    pitch = width * bytes_per_pixel;
    pitch = (pitch + 3) & ~3;
    if (height > (INT_MAX - FB_RISCOS_SPRITE_PIXEL_OFFSET) / pitch)
        return -1;

    size = FB_RISCOS_SPRITE_PIXEL_OFFSET + (pitch * height);
    area = (FB_RISCOS_SPRITE_AREA *)malloc((size_t)size);
    if (area == NULL)
        return -1;

    memset(area, 0, (size_t)size);
    area->size = (unsigned int)size;
    area->start = FB_RISCOS_SPRITE_AREA_HEADER_SIZE;

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = FB_RISCOS_SPRITE_INITIALISE;
    registers.r[1] = (int)area;
    if (riscos_window_swi(OS_SpriteOp, &registers,
        "OS_SpriteOp initialise") != 0)
    {
        free(area);
        return -1;
    }

    mode = riscos_window_read_mode();
    if (mode < 0)
    {
        free(area);
        return -1;
    }

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = FB_RISCOS_SPRITE_CREATE;
    registers.r[1] = (int)area;
    registers.r[2] = (int)sprite_name;
    registers.r[3] = 0;
    registers.r[4] = width;
    registers.r[5] = height;
    registers.r[6] = mode;
    if (riscos_window_swi(OS_SpriteOp, &registers,
        "OS_SpriteOp create") != 0)
    {
        free(area);
        return -1;
    }

    fb_riscos_gfx.sprite_area = (unsigned char *)area;
    fb_riscos_gfx.sprite_pixels = fb_riscos_gfx.sprite_area +
        FB_RISCOS_SPRITE_PIXEL_OFFSET;
    fb_riscos_gfx.sprite_area_size = size;
    fb_riscos_gfx.sprite_pitch = pitch;
    return 0;
}

static int riscos_window_create_sprite_translation(void)
{
    _kernel_swi_regs registers;
    int translation_size;

    if (fb_riscos_gfx.sprite_area == NULL)
        return -1;

    /*
        An 8bpp desktop does not interpret a user sprite's byte values as
        the physical VIDC colour numbers that gfxlib2 stores.  ColourTrans
        supplies the table which maps the sprite's source palette into the
        current Wimp palette.  OS_SpriteOp 52 consumes that table directly.

        Asking ColourTrans to describe the actual sprite, rather than a
        guessed display mode, also keeps the 16bpp and 32bpp paths correct
        if a future RISC OS version uses a mode selector with extra flags.
    */

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = (int)fb_riscos_gfx.sprite_area;
    registers.r[1] = (int)fb_riscos_gfx.sprite_area +
        FB_RISCOS_SPRITE_AREA_HEADER_SIZE;
    registers.r[2] = -1;
    registers.r[3] = -1;
    registers.r[4] = 0;
    registers.r[5] = 3;
    if (riscos_window_swi(ColourTrans_SelectTable, &registers,
        "ColourTrans_SelectTable size") != 0)
    {
        return -1;
    }

    translation_size = registers.r[4];
    if (translation_size <= 0 ||
        translation_size > FB_RISCOS_SPRITE_TRANSLATION_LIMIT)
    {
        fb_riscosGfxDebug("invalid sprite translation size %d",
            translation_size);
        return -1;
    }

    fb_riscos_gfx.sprite_translation =
        (unsigned char *)malloc((size_t)translation_size);
    if (fb_riscos_gfx.sprite_translation == NULL)
        return -1;

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = (int)fb_riscos_gfx.sprite_area;
    registers.r[1] = (int)fb_riscos_gfx.sprite_area +
        FB_RISCOS_SPRITE_AREA_HEADER_SIZE;
    registers.r[2] = -1;
    registers.r[3] = -1;
    registers.r[4] = (int)fb_riscos_gfx.sprite_translation;
    registers.r[5] = 3;
    if (riscos_window_swi(ColourTrans_SelectTable, &registers,
        "ColourTrans_SelectTable") != 0)
    {
        free(fb_riscos_gfx.sprite_translation);
        fb_riscos_gfx.sprite_translation = NULL;
        return -1;
    }

    return 0;
}

static void riscos_window_convert_line_32(unsigned char *destination,
    const unsigned char *source, int width)
{
    unsigned int *output;
    int x;

    output = (unsigned int *)destination;
    for (x = 0; x < width; ++x)
        output[x] = fb_riscosGfxSourceRgb(x, source);
}

static void riscos_window_convert_line_16(unsigned char *destination,
    const unsigned char *source, int width)
{
    unsigned short *output;
    unsigned int blue;
    unsigned int green;
    unsigned int red;
    unsigned int rgb;
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

static unsigned char riscos_window_colour_number(unsigned int rgb)
{
    _kernel_swi_regs registers;
    unsigned int blue;
    unsigned int cache_index;
    unsigned int green;
    unsigned int red;

    /*
        A 256-colour Wimp desktop is an ordinary and useful configuration.
        ColourTrans owns its palette policy, so convert the 12-bit RGB cache
        keys through it rather than assuming a particular desktop palette.
    */

    red = rgb & 255U;
    green = (rgb >> 8) & 255U;
    blue = (rgb >> 16) & 255U;
    cache_index = (red >> 4) | ((green >> 4) << 4) | ((blue >> 4) << 8);

    if (fb_riscos_gfx.window_colour_cache_valid[cache_index])
        return fb_riscos_gfx.window_colour_cache[cache_index];

    red = (cache_index & 15U) * 17U;
    green = ((cache_index >> 4) & 15U) * 17U;
    blue = ((cache_index >> 8) & 15U) * 17U;
    memset(&registers, 0, sizeof(registers));
    registers.r[0] = (int)((red | (green << 8) | (blue << 16)) << 8);

    if (riscos_window_swi(ColourTrans_ReturnColourNumber, &registers,
        "ColourTrans_ReturnColourNumber") != 0)
    {
        fb_riscos_gfx.window_colour_cache[cache_index] = 0;
    }
    else
    {
        fb_riscos_gfx.window_colour_cache[cache_index] =
            (unsigned char)registers.r[0];
    }

    fb_riscos_gfx.window_colour_cache_valid[cache_index] = 1;
    return fb_riscos_gfx.window_colour_cache[cache_index];
}

static void riscos_window_convert_line_8(unsigned char *destination,
    const unsigned char *source, int width)
{
    unsigned int rgb;
    int x;

    for (x = 0; x < width; ++x)
    {
        rgb = fb_riscosGfxSourceRgb(x, source);
        destination[x] = riscos_window_colour_number(rgb);
    }
}

static void riscos_window_copy_dirty_lines(void)
{
    unsigned char *destination;
    const unsigned char *source;
    int y;

    if (__fb_gfx == NULL || __fb_gfx->framebuffer == NULL ||
        fb_riscos_gfx.sprite_pixels == NULL)
    {
        return;
    }

    source = __fb_gfx->framebuffer;
    for (y = 0; y < fb_riscos_gfx.viewport_height; ++y)
    {
        if (__fb_gfx->dirty != NULL && !__fb_gfx->dirty[y])
        {
            source += __fb_gfx->pitch;
            continue;
        }

        /*
            OS_SpriteOp presents the first stored row at the top of the
            sprite.  Keeping the gfxlib2 top-to-bottom order here is also
            what preserves text and game artwork without vertical reversal.
        */
        destination = fb_riscos_gfx.sprite_pixels +
            (y * fb_riscos_gfx.sprite_pitch);
        if (fb_riscos_gfx.screen_depth == 32)
        {
            riscos_window_convert_line_32(destination, source,
                fb_riscos_gfx.viewport_width);
        }
        else if (fb_riscos_gfx.screen_depth == 16)
        {
            riscos_window_convert_line_16(destination, source,
                fb_riscos_gfx.viewport_width);
        }
        else
        {
            riscos_window_convert_line_8(destination, source,
                fb_riscos_gfx.viewport_width);
        }

        if (__fb_gfx->dirty != NULL)
            __fb_gfx->dirty[y] = FALSE;
        source += __fb_gfx->pitch;
    }
}

/* ------------------------------------------------------------------------- */
/* Wimp redraw and update sequences                                          */
/* ------------------------------------------------------------------------- */

static int riscos_window_plot_sprite(void)
{
    _kernel_swi_regs registers;
    long long plot_x;
    long long plot_y;
    long long work_height_units;

    if (fb_riscos_gfx.sprite_area == NULL ||
        fb_riscos_gfx.sprite_translation == NULL)
    {
        return -1;
    }

    /*
        Wimp_RedrawWindow and Wimp_UpdateWindow establish a clip rectangle,
        but do not change OS_SpriteOp into work-area coordinates.  The Wimp
        redraw block tells us where work coordinate (0, 0) lies on screen:

            screen_x = visible_x0 - scroll_x
            screen_y = visible_y1 - scroll_y

        Sprites are anchored at their lower-left corner.  Our work area has
        its conventional origin at the upper-left, therefore the sprite's
        lower-left corner is one work-area height below that origin.
    */
    work_height_units = (long long)fb_riscos_gfx.viewport_height <<
        fb_riscos_gfx.y_eigen;
    plot_x = (long long)fb_riscos_gfx.wimp_visible_x0 -
        fb_riscos_gfx.wimp_scroll_x;
    plot_y = (long long)fb_riscos_gfx.wimp_visible_y1 -
        fb_riscos_gfx.wimp_scroll_y - work_height_units;
    if (plot_x < INT_MIN || plot_x > INT_MAX ||
        plot_y < INT_MIN || plot_y > INT_MAX)
    {
        fb_riscosGfxDebug("Wimp sprite position outside OS coordinate range");
        return -1;
    }

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = FB_RISCOS_SPRITE_PLOT;
    registers.r[1] = (int)fb_riscos_gfx.sprite_area;
    registers.r[2] = (int)fb_riscos_gfx.sprite_area +
        FB_RISCOS_SPRITE_AREA_HEADER_SIZE;
    registers.r[3] = (int)plot_x;
    registers.r[4] = (int)plot_y;
    registers.r[5] = 0;
    registers.r[6] = 0;
    registers.r[7] = (int)fb_riscos_gfx.sprite_translation;

    return riscos_window_swi(OS_SpriteOp, &registers, "OS_SpriteOp plot");
}

static int riscos_window_redraw(int swi, FB_RISCOS_WIMP_BLOCK *block,
    const char *operation)
{
    _kernel_swi_regs registers;
    int more;

    memset(&registers, 0, sizeof(registers));
    registers.r[1] = (int)block;
    if (riscos_window_swi(swi, &registers, operation) != 0)
        return -1;

    riscos_window_store_state(block);
    more = registers.r[0];
    while (more != 0)
    {
        if (riscos_window_plot_sprite() != 0)
            return -1;

        memset(&registers, 0, sizeof(registers));
        registers.r[1] = (int)block;
        if (riscos_window_swi(Wimp_GetRectangle, &registers,
            "Wimp_GetRectangle") != 0)
        {
            return -1;
        }

        riscos_window_store_state(block);
        more = registers.r[0];
    }

    return 0;
}

static void riscos_window_handle_redraw(FB_RISCOS_WIMP_BLOCK *block)
{
    if (block->words[0] != fb_riscos_gfx.wimp_window_handle)
        return;

    (void)riscos_window_redraw(Wimp_RedrawWindow, block,
        "Wimp_RedrawWindow");
}

/* ------------------------------------------------------------------------- */
/* Wimp event processing                                                     */
/* ------------------------------------------------------------------------- */

static void riscos_window_post_close(void)
{
    EVENT event;

    if (!fb_riscos_gfx.active)
        return;

    fb_riscos_gfx.active = 0;
    riscos_window_post_focus(FALSE);
    memset(&event, 0, sizeof(event));
    event.type = EVENT_WINDOW_CLOSE;
    fb_hPostEvent(&event);
    fb_hPostKey(KEY_QUIT);
}

static void riscos_window_handle_event(int event_code,
    FB_RISCOS_WIMP_BLOCK *block)
{
    int key;

    switch (event_code)
    {
        case FB_RISCOS_WIMP_REDRAW:
            riscos_window_handle_redraw(block);
            break;

        case FB_RISCOS_WIMP_OPEN:
            if (block->words[0] == fb_riscos_gfx.wimp_window_handle)
                (void)riscos_window_open(block);
            break;

        case FB_RISCOS_WIMP_CLOSE:
            if (block->words[0] == fb_riscos_gfx.wimp_window_handle)
            {
                riscos_window_close();
                riscos_window_post_close();
            }
            break;

        case FB_RISCOS_WIMP_MOUSE_CLICK:
            if (block->words[3] == fb_riscos_gfx.wimp_window_handle)
                riscos_window_set_focus();
            break;

        case FB_RISCOS_WIMP_KEY:
            key = fb_riscosGfxTranslateCharacter(block->words[6]);
            if (key != 0)
                fb_hPostKey(key);
            break;

        case FB_RISCOS_WIMP_LOSE_CARET:
            if (block->words[0] == fb_riscos_gfx.wimp_window_handle)
                riscos_window_post_focus(FALSE);
            break;

        case FB_RISCOS_WIMP_GAIN_CARET:
            if (block->words[0] == fb_riscos_gfx.wimp_window_handle)
                riscos_window_post_focus(TRUE);
            break;

        case FB_RISCOS_WIMP_USER_MESSAGE:
        case FB_RISCOS_WIMP_USER_MESSAGE_RECORDED:
            if (block->words[4] == 0)
            {
                riscos_window_close();
                riscos_window_post_close();
            }
            break;

        default:
            break;
    }

}

/* ------------------------------------------------------------------------- */
/* Public window lifecycle                                                   */
/* ------------------------------------------------------------------------- */

int fb_riscosGfxWindowInit(const char *title, int width, int height)
{
    FB_RISCOS_WIMP_BLOCK block;
    FB_RISCOS_WIMP_BLOCK open_block;
    _kernel_swi_regs registers;
    int screen_height_units;
    int screen_width_units;
    int work_height_units;
    int work_width_units;
    static const char task_name[] = "FreeBASIC";

    if (width <= 0 || height <= 0)
    {
        return -1;
    }

    /* Wimp_Initialise selects the configured desktop mode if necessary. */

    memset(&registers, 0, sizeof(registers));
    registers.r[0] = FB_RISCOS_WIMP_VERSION;
    registers.r[1] = FB_RISCOS_WIMP_TASK_MAGIC;
    registers.r[2] = (int)task_name;
    registers.r[3] = 0;
    if (riscos_window_swi(Wimp_Initialise, &registers,
        "Wimp_Initialise") != 0)
    {
        free(fb_riscos_gfx.sprite_area);
        fb_riscos_gfx.sprite_area = NULL;
        return -1;
    }

    fb_riscos_gfx.wimp_task_handle = registers.r[1];

    if (fb_riscosGfxReadVduVariables() != 0 ||
        width > (INT_MAX >> fb_riscos_gfx.x_eigen) ||
        height > (INT_MAX >> fb_riscos_gfx.y_eigen))
    {
        fb_riscosGfxWindowExit();
        return -1;
    }

    work_width_units = width << fb_riscos_gfx.x_eigen;
    work_height_units = height << fb_riscos_gfx.y_eigen;
    screen_width_units = fb_riscos_gfx.screen_width <<
        fb_riscos_gfx.x_eigen;
    screen_height_units = fb_riscos_gfx.screen_height <<
        fb_riscos_gfx.y_eigen;

    if (work_width_units > screen_width_units ||
        work_height_units > screen_height_units ||
        riscos_window_create_sprite(width, height) != 0 ||
        riscos_window_create_sprite_translation() != 0)
    {
        fb_riscosGfxWindowExit();
        return -1;
    }

    memset(&block, 0, sizeof(block));
    block.words[0] = (screen_width_units - work_width_units) / 2;
    block.words[1] = (screen_height_units - work_height_units) / 2;
    block.words[2] = block.words[0] + work_width_units;
    block.words[3] = block.words[1] + work_height_units;
    block.words[4] = 0;
    block.words[5] = 0;
    block.words[6] = -1;
    /*
        The high control-icon bits select the modern Close and Title Bar
        layout.  Moveability remains the low Wimp window-attribute bit, so
        it must be set independently or the title bar cannot be dragged.
    */
    block.words[7] = (int)(FB_RISCOS_WIMP_WINDOW_FLAGS |
        FB_RISCOS_WIMP_WINDOW_FLAG_MOVEABLE);
    block.bytes[32] = 7;
    block.bytes[33] = 1;
    block.bytes[34] = 7;
    block.bytes[35] = 0;
    block.bytes[36] = 1;
    block.bytes[37] = 2;
    block.bytes[38] = 12;
    block.words[10] = 0;
    block.words[11] = -work_height_units;
    block.words[12] = work_width_units;
    block.words[13] = 0;
    block.words[14] = (int)FB_RISCOS_WIMP_TITLE_FLAGS;
    block.words[15] = (int)FB_RISCOS_WIMP_WORK_BUTTON_FOCUS;
    block.words[16] = 0;
    block.words[17] = (work_width_units & 0xFFFF) |
        ((work_height_units & 0xFFFF) << 16);
    if (title == NULL || title[0] == '\0')
        title = "FreeBASIC";
    strncpy((char *)&block.bytes[72], title, 11);
    block.bytes[83] = '\0';

    memset(&registers, 0, sizeof(registers));
    registers.r[1] = (int)&block;
    if (riscos_window_swi(Wimp_CreateWindow, &registers,
        "Wimp_CreateWindow") != 0)
    {
        fb_riscosGfxWindowExit();
        return -1;
    }

    fb_riscos_gfx.wimp_window_handle = registers.r[0];
    memset(&open_block, 0, sizeof(open_block));
    open_block.words[0] = fb_riscos_gfx.wimp_window_handle;
    open_block.words[1] = (screen_width_units - work_width_units) / 2;
    open_block.words[2] = (screen_height_units - work_height_units) / 2;
    open_block.words[3] = open_block.words[1] + work_width_units;
    open_block.words[4] = open_block.words[2] + work_height_units;
    open_block.words[5] = 0;
    open_block.words[6] = 0;
    open_block.words[7] = -1;
    if (riscos_window_open(&open_block) != 0)
    {
        fb_riscosGfxWindowExit();
        return -1;
    }

    fb_riscos_gfx.viewport_x = 0;
    fb_riscos_gfx.viewport_y = 0;
    fb_riscos_gfx.viewport_width = width;
    fb_riscos_gfx.viewport_height = height;
    fb_riscos_gfx.windowed = 1;
    fb_riscos_gfx.active = 1;
    fb_riscos_gfx.mouse_clip = FALSE;

    riscos_window_set_focus();
    fb_riscosGfxDebug("opened Wimp window %d at %dx%d",
        fb_riscos_gfx.wimp_window_handle, width, height);
    return 0;
}

void fb_riscosGfxWindowExit(void)
{
    FB_RISCOS_WIMP_BLOCK block;
    _kernel_swi_regs registers;

    riscos_window_close();

    if (fb_riscos_gfx.wimp_window_handle >= 0)
    {
        memset(&block, 0, sizeof(block));
        memset(&registers, 0, sizeof(registers));
        block.words[0] = fb_riscos_gfx.wimp_window_handle;
        registers.r[1] = (int)&block;
        (void)riscos_window_swi(Wimp_DeleteWindow, &registers,
            "Wimp_DeleteWindow");
    }

    if (fb_riscos_gfx.wimp_task_handle >= 0)
    {
        memset(&registers, 0, sizeof(registers));
        registers.r[0] = fb_riscos_gfx.wimp_task_handle;
        registers.r[1] = FB_RISCOS_WIMP_TASK_MAGIC;
        (void)riscos_window_swi(Wimp_CloseDown, &registers,
            "Wimp_CloseDown");
    }

    free(fb_riscos_gfx.sprite_area);
    fb_riscos_gfx.sprite_area = NULL;
    fb_riscos_gfx.sprite_pixels = NULL;
    free(fb_riscos_gfx.sprite_translation);
    fb_riscos_gfx.sprite_translation = NULL;
    fb_riscos_gfx.sprite_area_size = 0;
    fb_riscos_gfx.sprite_pitch = 0;
    fb_riscos_gfx.wimp_window_handle = -1;
    fb_riscos_gfx.wimp_task_handle = -1;
    fb_riscos_gfx.wimp_window_open = 0;
    fb_riscos_gfx.wimp_has_input_focus = FALSE;
    fb_riscos_gfx.active = 0;
}

void fb_riscosGfxWindowPresent(void)
{
    FB_RISCOS_WIMP_BLOCK block;

    if (!fb_riscos_gfx.active || !fb_riscos_gfx.wimp_window_open)
        return;

    /*
        A graphics program may spend long periods drawing frames without
        asking for keyboard or mouse input.  Servicing the Wimp queue before
        each presentation lets title-bar drags, close requests, and redraws
        reach their normal Wimp handlers during those loops.  The redraw
        handler calls OS_SpriteOp directly, so it cannot recurse into this
        present path.
    */
    fb_riscosGfxWindowPollEvents();
    if (!fb_riscos_gfx.active || !fb_riscos_gfx.wimp_window_open)
        return;

    riscos_window_copy_dirty_lines();

    memset(&block, 0, sizeof(block));
    block.words[0] = fb_riscos_gfx.wimp_window_handle;
    block.words[1] = 0;
    block.words[2] = -(fb_riscos_gfx.viewport_height <<
        fb_riscos_gfx.y_eigen);
    block.words[3] = fb_riscos_gfx.viewport_width <<
        fb_riscos_gfx.x_eigen;
    block.words[4] = 0;
    (void)riscos_window_redraw(Wimp_UpdateWindow, &block,
        "Wimp_UpdateWindow");
}

void fb_riscosGfxWindowPollEvents(void)
{
    FB_RISCOS_WIMP_BLOCK block;
    _kernel_swi_regs registers;
    int count;

    if (!fb_riscos_gfx.windowed || fb_riscos_gfx.wimp_task_handle < 0)
        return;

    for (count = 0; count < FB_RISCOS_WINDOW_POLL_LIMIT; ++count)
    {
        memset(&block, 0, sizeof(block));
        memset(&registers, 0, sizeof(registers));
        registers.r[0] = FB_RISCOS_WIMP_POLL_MASK;
        registers.r[1] = (int)&block;
        registers.r[2] = riscos_window_read_time();
        registers.r[3] = 0;
        if (riscos_window_swi(Wimp_PollIdle, &registers,
            "Wimp_PollIdle") != 0)
        {
            return;
        }

        if (registers.r[0] == 0)
            return;

        riscos_window_handle_event(registers.r[0], &block);
    }
}

/* ------------------------------------------------------------------------- */
/* Public window input and inspection                                        */
/* ------------------------------------------------------------------------- */

static int riscos_window_clamp(int value, int lower, int upper)
{
    if (value < lower)
        return lower;
    if (value > upper)
        return upper;
    return value;
}

static void riscos_window_poll_mouse(void)
{
    FB_RISCOS_WIMP_BLOCK block;
    _kernel_swi_regs registers;
    EVENT event;
    int buttons;
    int old_buttons;
    int old_x;
    int old_y;
    int work_x;
    int work_y;

    /*
        Wimp_GetPointerInfo returns the desktop's instantaneous pointer and
        button state in the same coordinate system used by this window.  It
        avoids the queued OS_Mouse records that can lag behind a Wimp task.
    */

    memset(&block, 0, sizeof(block));
    memset(&registers, 0, sizeof(registers));
    registers.r[1] = (int)&block;
    if (riscos_window_swi(Wimp_GetPointerInfo, &registers,
        "Wimp_GetPointerInfo") != 0)
    {
        return;
    }

    work_x = block.words[0] + fb_riscos_gfx.wimp_scroll_x -
        fb_riscos_gfx.wimp_visible_x0;
    work_y = block.words[1] + fb_riscos_gfx.wimp_scroll_y -
        fb_riscos_gfx.wimp_visible_y1;
    old_x = fb_riscos_gfx.mouse_x;
    old_y = fb_riscos_gfx.mouse_y;
    old_buttons = fb_riscos_gfx.mouse_buttons;

    fb_riscos_gfx.mouse_x = riscos_window_clamp(
        work_x >> fb_riscos_gfx.x_eigen, 0,
        fb_riscos_gfx.viewport_width - 1);
    fb_riscos_gfx.mouse_y = riscos_window_clamp(
        (-work_y) >> fb_riscos_gfx.y_eigen, 0,
        fb_riscos_gfx.viewport_height - 1);
    buttons = block.words[2];
    fb_riscos_gfx.mouse_buttons =
        ((buttons & 4) ? BUTTON_LEFT : 0) |
        ((buttons & 1) ? BUTTON_RIGHT : 0) |
        ((buttons & 2) ? BUTTON_MIDDLE : 0);

    if (old_x != fb_riscos_gfx.mouse_x || old_y != fb_riscos_gfx.mouse_y)
    {
        memset(&event, 0, sizeof(event));
        event.type = EVENT_MOUSE_MOVE;
        event.x = fb_riscos_gfx.mouse_x;
        event.y = fb_riscos_gfx.mouse_y;
        event.dx = fb_riscos_gfx.mouse_x - old_x;
        event.dy = fb_riscos_gfx.mouse_y - old_y;
        fb_hPostEvent(&event);
    }

    buttons = old_buttons ^ fb_riscos_gfx.mouse_buttons;
    for (event.button = BUTTON_LEFT; event.button <= BUTTON_MIDDLE;
        event.button <<= 1)
    {
        if (buttons & event.button)
        {
            int button;

            button = event.button;
            memset(&event, 0, sizeof(event));
            event.button = button;
            event.type = (fb_riscos_gfx.mouse_buttons & event.button)
                ? EVENT_MOUSE_BUTTON_PRESS
                : EVENT_MOUSE_BUTTON_RELEASE;
            fb_hPostEvent(&event);
        }
    }
}

int fb_riscosGfxWindowGetMouse(int *x, int *y, int *z, int *buttons,
    int *clip)
{
    if (!fb_riscos_gfx.active)
        return -1;

    fb_riscosGfxWindowPollEvents();
    riscos_window_poll_mouse();

    if (x != NULL)
        *x = fb_riscos_gfx.mouse_x;
    if (y != NULL)
        *y = fb_riscos_gfx.mouse_y;
    if (z != NULL)
        *z = fb_riscos_gfx.mouse_z;
    if (buttons != NULL)
        *buttons = fb_riscos_gfx.mouse_buttons;
    if (clip != NULL)
        *clip = fb_riscos_gfx.mouse_clip;

    return 0;
}

void fb_riscosGfxWindowSetMouse(int x, int y, int cursor, int clip)
{
    union
    {
        int aligned[2];
        unsigned char bytes[8];
    } parameters;
    int screen_x;
    int screen_y;

    if (!fb_riscos_gfx.active)
        return;

    if (clip >= 0)
        fb_riscos_gfx.mouse_clip = (clip != 0);

    /*
        Hiding the system pointer inside a Wimp window would leave no cursor
        above the game while other tasks own the screen.  Keep it Wimp-owned;
        the cursor argument is retained for source compatibility.
    */

    (void)cursor;

    if (x < 0 || y < 0)
        return;

    x = riscos_window_clamp(x, 0, fb_riscos_gfx.viewport_width - 1);
    y = riscos_window_clamp(y, 0, fb_riscos_gfx.viewport_height - 1);
    screen_x = (x << fb_riscos_gfx.x_eigen) -
        fb_riscos_gfx.wimp_scroll_x + fb_riscos_gfx.wimp_visible_x0;
    screen_y = -(y << fb_riscos_gfx.y_eigen) -
        fb_riscos_gfx.wimp_scroll_y + fb_riscos_gfx.wimp_visible_y1;

    memset(&parameters, 0, sizeof(parameters));
    parameters.bytes[0] = 3;
    parameters.bytes[1] = (unsigned char)(screen_x & 255);
    parameters.bytes[2] = (unsigned char)((screen_x >> 8) & 255);
    parameters.bytes[3] = (unsigned char)(screen_y & 255);
    parameters.bytes[4] = (unsigned char)((screen_y >> 8) & 255);
    (void)_kernel_osword(21, parameters.aligned);

    /*
        OS_Word 21,5 moves the pointer when it is unlinked from the mouse.
        Pass the same coordinates as sub-reason 3 so SETMOUSE behaves the
        same whether the desktop currently links those two positions or not.
    */

    parameters.bytes[0] = 5;
    (void)_kernel_osword(21, parameters.aligned);
    fb_riscos_gfx.mouse_x = x;
    fb_riscos_gfx.mouse_y = y;
}

unsigned int fb_riscosGfxWindowReadPresentedPixel(int x, int y)
{
    const unsigned char *address;
    _kernel_swi_regs registers;
    unsigned int palette[256];
    unsigned int blue;
    unsigned int green;
    unsigned int pixel;
    unsigned int red;

    if (!fb_riscos_gfx.active || x < 0 || y < 0 ||
        x >= fb_riscos_gfx.viewport_width ||
        y >= fb_riscos_gfx.viewport_height ||
        fb_riscos_gfx.sprite_pixels == NULL)
    {
        return 0;
    }

    address = fb_riscos_gfx.sprite_pixels +
        (y * fb_riscos_gfx.sprite_pitch) +
        (x * (fb_riscos_gfx.screen_depth / 8));
    if (fb_riscos_gfx.screen_depth == 32)
    {
        pixel = *((const unsigned int *)address);
        red = pixel & 255U;
        green = (pixel >> 8) & 255U;
        blue = (pixel >> 16) & 255U;
    }
    else if (fb_riscos_gfx.screen_depth == 16)
    {
        pixel = *((const unsigned short *)address);
        red = ((pixel >> 0) & 31U) * 255U / 31U;
        green = ((pixel >> 5) & 31U) * 255U / 31U;
        blue = ((pixel >> 10) & 31U) * 255U / 31U;
    }
    else
    {
        /*
            The byte in an 8bpp sprite is a ColourTrans colour number, not
            an OS_ReadPalette logical-colour register.  On VIDC desktops a
            GCOL value also carries tint bits, so using OS_ReadPalette after
            ColourNumberToGCOL can select a different palette register.

            ColourTrans_ReadPalette is the documented palette query for
            sprite colour numbers.  It supplies all 256 entries as standard
            &BBGGRR00 RGB words, in the exact ordering used by the sprite.
            This diagnostic path is intentionally outside the present loop;
            callers use it only for smoke-test assertions.
        */

        memset(palette, 0, sizeof(palette));
        memset(&registers, 0, sizeof(registers));
        registers.r[0] = -1;
        registers.r[1] = -1;
        registers.r[2] = (int)palette;
        registers.r[3] = (int)sizeof(palette);
        if (riscos_window_swi(ColourTrans_ReadPalette, &registers,
            "ColourTrans_ReadPalette") != 0)
        {
            return 0;
        }

        pixel = palette[*address];
        red = (pixel >> 8) & 255U;
        green = (pixel >> 16) & 255U;
        blue = (pixel >> 24) & 255U;
    }

    return 0xFF000000U | (red << 16) | (green << 8) | blue;
}

int fb_riscosGfxWindowHasInputFocus(void)
{
    return fb_riscos_gfx.wimp_has_input_focus;
}

ssize_t fb_riscosGfxWindowHandle(void)
{
    if (!fb_riscos_gfx.windowed)
        return 0;

    return fb_riscos_gfx.wimp_window_handle;
}

/* end of gfx_window.c */
