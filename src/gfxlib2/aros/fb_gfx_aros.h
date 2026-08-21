/*
    FreeBASIC gfxlib2 support for AROS
    ----------------------------------

    File: fb_gfx_aros.h

    Purpose:

        Define the private contract shared by the native AROS gfxlib2 backend.

    Responsibilities:

        - describe the Intuition window and CyberGraphX presentation state
        - declare display, input, and diagnostic services
        - keep AROS SDK declarations out of portable gfxlib2 sources

    This file intentionally does NOT contain:

        - generic drawing primitives
        - AROS implementation branches for other targets
        - direct framebuffer ownership
*/

#ifndef FB_GFX_AROS_H
#define FB_GFX_AROS_H

#include "../fb_gfx.h"

#include <exec/types.h>
#include <intuition/intuition.h>

typedef struct FB_AROS_GFX_STATE
{
    struct Screen *screen;
    struct Window *window;
    unsigned char *present_buffer;
    size_t present_buffer_size;
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
} FB_AROS_GFX_STATE;

extern FB_AROS_GFX_STATE fb_aros_gfx;

void fb_arosGfxDebug(const char *format, ...);

int fb_arosGfxDisplayInit(const char *title, int width, int height,
    int refresh_rate, int flags);
void fb_arosGfxDisplayExit(void);
void fb_arosGfxPresent(void);
void fb_arosGfxWaitVSync(void);
void fb_arosGfxSetWindowTitle(char *title);
int fb_arosGfxSetWindowPosition(int x, int y);
void fb_arosGfxReadScreenInfo(ssize_t *width, ssize_t *height,
    ssize_t *depth, ssize_t *refresh);

void fb_arosGfxInputInit(void);
void fb_arosGfxInputExit(void);
void fb_arosGfxPollEvents(void);
int fb_arosGfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip);
void fb_arosGfxSetMouse(int x, int y, int cursor, int clip);

#endif

/* end of fb_gfx_aros.h */
