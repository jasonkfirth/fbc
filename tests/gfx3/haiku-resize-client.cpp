/*
    FreeBASIC graphics runtime tests
    File: haiku-resize-client.cpp
    Purpose: Request real Haiku client resizes from the BASIC smoke test.
    Responsibilities: Check native bitmap bounds and issue locked ResizeTo calls.
    This file contains no application event loop or framebuffer manipulation.
*/

#include <Window.h>
#include <Bitmap.h>
#include <stdint.h>
#include <limits.h>

/* Internal allocation entry point, also used by driver initialization. */
BBitmap *fb_hHaikuCreateBitmap(int width, int height);

extern "C" int fb_test_haiku_bitmap_boundaries(void)
{
    const int dimensions[][2] = {{0, 1}, {1, 0}, {-1, 1}, {INT_MAX, 1},
        {16777218, 1}};

    /* 2^24 + 1 cannot be represented exactly as BRect's float endpoint. */
    for (unsigned int index = 0; index < sizeof(dimensions) / sizeof(dimensions[0]); index++)
    {
        BBitmap *bitmap = fb_hHaikuCreateBitmap(dimensions[index][0], dimensions[index][1]);
        if (bitmap)
        {
            delete bitmap;
            return -1;
        }
    }
    return 0;
}

extern "C" int fb_test_haiku_resize_client(int64_t handle, int width, int height)
{
    BWindow *window = reinterpret_cast<BWindow *>(static_cast<intptr_t>(handle));

    if (!window || width <= 0 || height <= 0 || !window->Lock())
        return -1;

    /* Haiku's client rectangle includes its right and bottom endpoints. */
    window->ResizeTo(width - 1, height - 1);
    window->Unlock();
    return 0;
}

/* end of haiku-resize-client.cpp */
