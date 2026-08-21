/*
    FreeBASIC gfxlib2 support for RISC OS
    -------------------------------------

    File: fb_gfx_riscos.h

    Purpose:

        Define the private contract shared by the RISC OS gfxlib2 backend.

    Responsibilities:

        - describe the selected physical display and logical viewport
        - declare display lifecycle and presentation helpers
        - declare the private Wimp window presentation boundary
        - declare native keyboard and pointer helpers
        - expose private presentation probes for backend acceptance tests

    This file intentionally does NOT contain:

        - generic gfxlib2 state
        - SWI implementations
        - platform driver registration
*/

#ifndef FB_GFX_RISCOS_H
#define FB_GFX_RISCOS_H

#include "../fb_gfx.h"

void fb_riscosGfxDebug(const char *format, ...);

#define FB_RISCOS_MODE_SPEC_WORDS          64
#define FB_RISCOS_WINDOW_COLOUR_CACHE_SIZE 4096

typedef struct FB_RISCOS_GFX_STATE
{
    unsigned char *screen;
    int screen_width;
    int screen_height;
    int screen_depth;
    int screen_pitch;
    int screen_size;
    int x_eigen;
    int y_eigen;

    int viewport_x;
    int viewport_y;
    int viewport_width;
    int viewport_height;

    int active;
    int cursors_removed;
    int mode_changed;
    int original_mode_words;
    int original_mode[FB_RISCOS_MODE_SPEC_WORDS];

    int pointer_setting_saved;
    int original_pointer_setting;
    int current_pointer_setting;

    int mouse_x;
    int mouse_y;
    int mouse_z;
    int mouse_buttons;
    int mouse_clip;

    /*
        Windowed presentation owns a Wimp task and a user sprite area.  The
        sprite lets the Wimp clip drawing against other desktop windows while
        gfxlib2 retains its normal software framebuffer.  On an 8bpp desktop,
        the cache records the ColourTrans result for each 12-bit RGB colour.
    */

    int windowed;
    int wimp_task_handle;
    int wimp_window_handle;
    int wimp_window_open;
    int wimp_has_input_focus;
    int wimp_visible_x0;
    int wimp_visible_y1;
    int wimp_scroll_x;
    int wimp_scroll_y;
    unsigned char *sprite_area;
    unsigned char *sprite_pixels;
    unsigned char *sprite_translation;
    int sprite_area_size;
    int sprite_pitch;
    unsigned char window_colour_cache[FB_RISCOS_WINDOW_COLOUR_CACHE_SIZE];
    unsigned char window_colour_cache_valid[
        FB_RISCOS_WINDOW_COLOUR_CACHE_SIZE];
} FB_RISCOS_GFX_STATE;

extern FB_RISCOS_GFX_STATE fb_riscos_gfx;

int fb_riscosGfxDisplayInit(const char *title, int width, int height,
    int refresh_rate, int flags);
void fb_riscosGfxDisplayExit(void);
void fb_riscosGfxPresent(void);
void fb_riscosGfxWaitVSync(void);
unsigned int fb_riscosGfxReadPresentedPixel(int x, int y);
int fb_riscosGfxPointerIsVisible(void);
void fb_riscosGfxReadScreenInfo(ssize_t *width, ssize_t *height,
    ssize_t *depth, ssize_t *refresh);

int fb_riscosGfxReadVduVariables(void);
unsigned int fb_riscosGfxSourceRgb(int x, const unsigned char *source);
unsigned int fb_riscosGfxReadSourcePixel(int x, int y);

int fb_riscosGfxWindowInit(const char *title, int width, int height);
void fb_riscosGfxWindowExit(void);
void fb_riscosGfxWindowPresent(void);
void fb_riscosGfxWindowPollEvents(void);
int fb_riscosGfxWindowGetMouse(int *x, int *y, int *z, int *buttons,
    int *clip);
void fb_riscosGfxWindowSetMouse(int x, int y, int cursor, int clip);
unsigned int fb_riscosGfxWindowReadPresentedPixel(int x, int y);
int fb_riscosGfxWindowHasInputFocus(void);
ssize_t fb_riscosGfxWindowHandle(void);

void fb_riscosGfxInputInit(void);
void fb_riscosGfxInputExit(void);
void fb_riscosGfxPollEvents(void);
int fb_riscosGfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip);
void fb_riscosGfxSetMouse(int x, int y, int cursor, int clip);
int fb_riscosGfxTranslateCharacter(int character);

#endif

/* end of fb_gfx_riscos.h */
