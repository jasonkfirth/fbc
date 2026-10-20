/*
    FreeBASIC gfxlib2 support for Windows CE
    ----------------------------------------

    File: gfx_display.c

    Purpose:

        Present the FreeBASIC software framebuffer in a native Windows CE
        GDI window.

    Responsibilities:

        - own the window class and top-level window
        - convert supported gfxlib2 pixel formats to a 32-bit GDI DIB
        - scale the DIB to the available client area
        - report screen geometry and native handles

    This file intentionally does NOT contain:

        - keyboard or mouse event translation
        - generic drawing operations
        - direct framebuffer or display-memory access

    Cursor model:

        GDI paints only the client surface.  Windows CE owns and composites
        the system cursor after each paint, so framebuffer updates cannot
        erase the pointer or require a task-window transition.
*/

#include "fb_gfx_wince.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define FB_WINCE_WINDOW_CLASS L"FreeBASICGfxWinCE"

FB_WINCE_GFX_STATE fb_wince_gfx;

static int wince_title_to_wide(const char *title, WCHAR *wide,
    int wide_count)
{
    int converted;

    if (wide == NULL || wide_count <= 0)
        return -1;

    wide[0] = L'\0';
    if (title == NULL || title[0] == '\0')
        title = "FreeBASIC";

    converted = MultiByteToWideChar(CP_ACP, 0, title, -1,
        wide, wide_count);
    if (converted <= 0)
    {
        wide[0] = L'F';
        wide[1] = L'B';
        wide[2] = L'\0';
        return -1;
    }

    wide[wide_count - 1] = L'\0';
    return 0;
}

static void wince_source_rgb(const unsigned char *source, int x,
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

    if (__fb_gfx->depth == 15)
    {
        pixel = ((const unsigned short *)source)[x];
        *red = (unsigned char)(((pixel >> 10) & 31U) * 255U / 31U);
        *green = (unsigned char)(((pixel >> 5) & 31U) * 255U / 31U);
        *blue = (unsigned char)((pixel & 31U) * 255U / 31U);
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

    if (__fb_gfx->depth == 24)
    {
        source += (size_t)x * 3U;
        *blue = source[0];
        *green = source[1];
        *red = source[2];
        return;
    }

    pixel = ((const unsigned int *)source)[x];
    *red = (unsigned char)((pixel >> 16) & 255U);
    *green = (unsigned char)((pixel >> 8) & 255U);
    *blue = (unsigned char)(pixel & 255U);
}

static void wince_convert_framebuffer(void)
{
    const unsigned char *source;
    unsigned char *destination;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    int source_y;
    int x;

    /*
        Positive DIB heights are bottom-up.  Filling rows in reverse order
        avoids depending on top-down DIB support in older Windows CE images.
    */
    for (source_y = 0; source_y < fb_wince_gfx.height; ++source_y)
    {
        source = __fb_gfx->framebuffer +
            ((size_t)source_y * (size_t)__fb_gfx->pitch);
        destination = fb_wince_gfx.present_buffer +
            ((size_t)(fb_wince_gfx.height - source_y - 1) *
             (size_t)fb_wince_gfx.width * 4U);

        for (x = 0; x < fb_wince_gfx.width; ++x)
        {
            wince_source_rgb(source, x, &red, &green, &blue);
            destination[0] = blue;
            destination[1] = green;
            destination[2] = red;
            destination[3] = 0;
            destination += 4;
        }
    }
}

void fb_winceGfxPresentToDevice(HDC device)
{
    RECT client;

    if (device == NULL || fb_wince_gfx.present_buffer == NULL ||
        __fb_gfx == NULL || __fb_gfx->framebuffer == NULL ||
        fb_wince_gfx.window == NULL)
    {
        return;
    }

    if (!GetClientRect(fb_wince_gfx.window, &client))
        return;
    if (client.right <= client.left || client.bottom <= client.top)
        return;

    wince_convert_framebuffer();
    StretchDIBits(device,
        0, 0, client.right - client.left, client.bottom - client.top,
        0, 0, fb_wince_gfx.width, fb_wince_gfx.height,
        fb_wince_gfx.present_buffer, &fb_wince_gfx.bitmap_info,
        DIB_RGB_COLORS, SRCCOPY);
}

void fb_winceGfxPresent(void)
{
    HDC device;
    int row;
    int dirty;

    if (!fb_wince_gfx.active || fb_wince_gfx.window == NULL ||
        __fb_gfx == NULL)
    {
        return;
    }

    dirty = (__fb_gfx->dirty == NULL);
    if (!dirty)
    {
        for (row = 0; row < fb_wince_gfx.height; ++row)
        {
            if (__fb_gfx->dirty[row])
            {
                dirty = TRUE;
                break;
            }
        }
    }

    if (!dirty)
        return;

    device = GetDC(fb_wince_gfx.window);
    if (device != NULL)
    {
        fb_winceGfxPresentToDevice(device);
        ReleaseDC(fb_wince_gfx.window, device);
    }

    if (__fb_gfx->dirty != NULL)
        memset(__fb_gfx->dirty, FALSE, (size_t)fb_wince_gfx.height);
}

int fb_winceGfxDisplayInit(const char *title, int width, int height,
    int refresh_rate, int flags)
{
    WNDCLASSW window_class;
    WCHAR wide_title[256];
    DWORD style;
    size_t pixels;
    int screen_width;
    int screen_height;
    int window_width;
    int window_height;

    memset(&fb_wince_gfx, 0, sizeof(fb_wince_gfx));

    if (width <= 0 || height <= 0)
        return -1;
    if ((size_t)width > SIZE_MAX / (size_t)height / 4U)
        return -1;

    pixels = (size_t)width * (size_t)height;
    fb_wince_gfx.present_buffer_size = pixels * 4U;
    fb_wince_gfx.present_buffer =
        (unsigned char *)malloc(fb_wince_gfx.present_buffer_size);
    if (fb_wince_gfx.present_buffer == NULL)
        return -1;
    memset(fb_wince_gfx.present_buffer, 0,
        fb_wince_gfx.present_buffer_size);

    fb_wince_gfx.instance = GetModuleHandleW(NULL);
    fb_wince_gfx.cursor = LoadCursorW(NULL, IDC_ARROW);

    memset(&window_class, 0, sizeof(window_class));
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    window_class.lpfnWndProc = fb_winceGfxWindowProc;
    window_class.hInstance = fb_wince_gfx.instance;
    window_class.hCursor = fb_wince_gfx.cursor;
    window_class.lpszClassName = FB_WINCE_WINDOW_CLASS;

    if (RegisterClassW(&window_class) == 0)
    {
        fb_winceGfxDisplayExit();
        return -1;
    }
    fb_wince_gfx.class_registered = TRUE;

    wince_title_to_wide(title, wide_title,
        (int)(sizeof(wide_title) / sizeof(wide_title[0])));

    screen_width = GetSystemMetrics(SM_CXSCREEN);
    screen_height = GetSystemMetrics(SM_CYSCREEN);
    if (screen_width <= 0 || screen_height <= 0)
    {
        fb_winceGfxDisplayExit();
        return -1;
    }

    if (flags & DRIVER_FULLSCREEN)
    {
        style = WS_VISIBLE | WS_POPUP;
        window_width = screen_width;
        window_height = screen_height;
    }
    else
    {
        style = WS_VISIBLE | WS_CAPTION | WS_SYSMENU;
        window_width = (width < screen_width) ? width : screen_width;
        window_height = (height < screen_height) ? height : screen_height;
    }

    fb_wince_gfx.window = CreateWindowExW(0, FB_WINCE_WINDOW_CLASS,
        wide_title, style, 0, 0, window_width, window_height,
        NULL, NULL, fb_wince_gfx.instance, NULL);
    if (fb_wince_gfx.window == NULL)
    {
        fb_winceGfxDisplayExit();
        return -1;
    }

    memset(&fb_wince_gfx.bitmap_info, 0,
        sizeof(fb_wince_gfx.bitmap_info));
    fb_wince_gfx.bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    fb_wince_gfx.bitmap_info.bmiHeader.biWidth = width;
    fb_wince_gfx.bitmap_info.bmiHeader.biHeight = height;
    fb_wince_gfx.bitmap_info.bmiHeader.biPlanes = 1;
    fb_wince_gfx.bitmap_info.bmiHeader.biBitCount = 32;
    fb_wince_gfx.bitmap_info.bmiHeader.biCompression = BI_RGB;
    fb_wince_gfx.bitmap_info.bmiHeader.biSizeImage =
        (DWORD)fb_wince_gfx.present_buffer_size;

    fb_wince_gfx.width = width;
    fb_wince_gfx.height = height;
    fb_wince_gfx.refresh_rate = (refresh_rate > 0) ? refresh_rate : 60;
    fb_wince_gfx.flags = flags;
    fb_wince_gfx.cursor_visible = TRUE;
    fb_wince_gfx.active = TRUE;

    ShowWindow(fb_wince_gfx.window, SW_SHOW);
    UpdateWindow(fb_wince_gfx.window);
    return 0;
}

void fb_winceGfxDisplayExit(void)
{
    fb_wince_gfx.active = FALSE;

    if (fb_wince_gfx.window != NULL)
    {
        DestroyWindow(fb_wince_gfx.window);
        fb_wince_gfx.window = NULL;
    }

    if (fb_wince_gfx.class_registered)
    {
        UnregisterClassW(FB_WINCE_WINDOW_CLASS, fb_wince_gfx.instance);
        fb_wince_gfx.class_registered = FALSE;
    }

    free(fb_wince_gfx.present_buffer);
    fb_wince_gfx.present_buffer = NULL;
    fb_wince_gfx.present_buffer_size = 0;
}

void fb_winceGfxWaitVSync(void)
{
    DWORD delay;

    delay = (fb_wince_gfx.refresh_rate > 0)
        ? (DWORD)(1000 / fb_wince_gfx.refresh_rate)
        : 16;
    if (delay == 0)
        delay = 1;
    Sleep(delay);
}

void fb_winceGfxSetWindowTitle(char *title)
{
    WCHAR wide_title[256];

    if (fb_wince_gfx.window == NULL || title == NULL)
        return;

    wince_title_to_wide(title, wide_title,
        (int)(sizeof(wide_title) / sizeof(wide_title[0])));
    SetWindowTextW(fb_wince_gfx.window, wide_title);
}

int fb_winceGfxSetWindowPosition(int x, int y)
{
    RECT rectangle;
    int width;
    int height;

    if (fb_wince_gfx.window == NULL ||
        !GetWindowRect(fb_wince_gfx.window, &rectangle))
    {
        return 0;
    }

    width = rectangle.right - rectangle.left;
    height = rectangle.bottom - rectangle.top;
    if (x != INT_MIN || y != INT_MIN)
    {
        if (x == INT_MIN)
            x = rectangle.left;
        if (y == INT_MIN)
            y = rectangle.top;
        MoveWindow(fb_wince_gfx.window, x, y, width, height, TRUE);
        GetWindowRect(fb_wince_gfx.window, &rectangle);
    }

    return (rectangle.left & 0xFFFF) | (rectangle.top << 16);
}

void fb_winceGfxReadScreenInfo(ssize_t *width, ssize_t *height,
    ssize_t *depth, ssize_t *refresh)
{
    HDC device;
    int bits;
    int planes;

    *width = GetSystemMetrics(SM_CXSCREEN);
    *height = GetSystemMetrics(SM_CYSCREEN);
    *depth = 0;
    *refresh = 0;

    device = GetDC(NULL);
    if (device == NULL)
        return;

    bits = GetDeviceCaps(device, BITSPIXEL);
    planes = GetDeviceCaps(device, PLANES);
    if (bits > 0 && planes > 0)
        *depth = (ssize_t)bits * (ssize_t)planes;
    bits = GetDeviceCaps(device, VREFRESH);
    if (bits > 0)
        *refresh = bits;
    ReleaseDC(NULL, device);
}

/* end of gfx_display.c */
