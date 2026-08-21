/*
    FreeBASIC gfxlib2 support for Windows CE
    ----------------------------------------

    File: fb_gfx_wince.h

    Purpose:

        Define the private contract shared by the native Windows CE gfxlib2
        backend.

    Responsibilities:

        - describe the GDI window and presentation state
        - declare display and input services
        - keep Windows CE declarations out of portable gfxlib2 sources

    This file intentionally does NOT contain:

        - generic drawing primitives
        - Windows desktop compatibility branches
        - sound or console runtime policy
*/

#ifndef FB_GFX_WINCE_H
#define FB_GFX_WINCE_H

#include "../fb_gfx.h"

#include <windows.h>

typedef struct FB_WINCE_GFX_STATE
{
    HINSTANCE instance;
    HWND window;
    HCURSOR cursor;
    BITMAPINFO bitmap_info;
    unsigned char *present_buffer;
    size_t present_buffer_size;
    int width;
    int height;
    int refresh_rate;
    int flags;
    int active;
    int class_registered;
    int mouse_x;
    int mouse_y;
    int mouse_z;
    int mouse_buttons;
    int mouse_clip;
    int cursor_visible;
} FB_WINCE_GFX_STATE;

extern FB_WINCE_GFX_STATE fb_wince_gfx;
extern const unsigned char __fb_keytable[][3];

int fb_winceGfxDisplayInit(const char *title, int width, int height,
    int refresh_rate, int flags);
void fb_winceGfxDisplayExit(void);
void fb_winceGfxPresent(void);
void fb_winceGfxPresentToDevice(HDC device);
void fb_winceGfxWaitVSync(void);
void fb_winceGfxSetWindowTitle(char *title);
int fb_winceGfxSetWindowPosition(int x, int y);
void fb_winceGfxReadScreenInfo(ssize_t *width, ssize_t *height,
    ssize_t *depth, ssize_t *refresh);

void fb_winceGfxInputInit(void);
void fb_winceGfxInputExit(void);
void fb_winceGfxPollEvents(void);
int fb_winceGfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip);
void fb_winceGfxSetMouse(int x, int y, int cursor, int clip);
LRESULT CALLBACK fb_winceGfxWindowProc(HWND window, UINT message,
    WPARAM wparam, LPARAM lparam);

#endif

/* end of fb_gfx_wince.h */
