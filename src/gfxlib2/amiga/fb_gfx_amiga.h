/*
    FreeBASIC gfxlib2 support for AmigaOS
    ----------------------------------

    File: fb_gfx_amiga.h

    Purpose:

        Define the private contract shared by the native AmigaOS gfxlib2 backend.

    Responsibilities:

        - describe the Intuition window and CyberGraphX presentation state
        - declare display, input, and diagnostic services
        - keep AMIGA SDK declarations out of portable gfxlib2 sources

    This file intentionally does NOT contain:

        - generic drawing primitives
        - AMIGA implementation branches for other targets
        - direct framebuffer ownership
*/

#ifndef FB_GFX_AMIGA_H
#define FB_GFX_AMIGA_H

#include "../fb_gfx.h"

#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>

typedef struct FB_AMIGA_GFX_STATE
{
    struct Screen *screen;
    struct Window *window;
    unsigned char *present_buffer;
    size_t present_buffer_size;
    struct BitMap *line_bitmap;
    struct RastPort line_rastport;
    int private_screen;
    int rtg;
    int screen_depth;
    int width;
    int height;
    int refresh_rate;
    int active;
    int mouse_x;
    int mouse_y;
    int mouse_z;
    int mouse_buttons;
    int mouse_clip;
    int cursor_visible;
} FB_AMIGA_GFX_STATE;

extern FB_AMIGA_GFX_STATE fb_amiga_gfx;

void fb_amigaGfxDebug(const char *format, ...);

int fb_amigaGfxDisplayInit(const char *title, int width, int height,
    int refresh_rate, int flags);
void fb_amigaGfxDisplayExit(void);
void fb_amigaGfxPresent(void);
void fb_amigaGfxSetPalette(int index, int red, int green, int blue);
void fb_amigaGfxWaitVSync(void);
void fb_amigaGfxSetWindowTitle(char *title);
int fb_amigaGfxSetWindowPosition(int x, int y);
void fb_amigaGfxReadScreenInfo(ssize_t *width, ssize_t *height,
    ssize_t *depth, ssize_t *refresh);

void fb_amigaGfxInputInit(void);
void fb_amigaGfxInputExit(void);
void fb_amigaGfxPollEvents(void);
int fb_amigaGfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip);
void fb_amigaGfxSetMouse(int x, int y, int cursor, int clip);

#endif

/* end of fb_gfx_amiga.h */
