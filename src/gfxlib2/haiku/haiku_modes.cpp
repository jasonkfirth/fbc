/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: haiku_modes.cpp

    Purpose:

        Enumerate display modes available to the graphics runtime.

    Responsibilities:

        • query the Haiku operating system for available display modes
        • expose those modes to gfxlib
        • provide legacy framebuffer resolutions used by FreeBASIC
        • guarantee a safe fallback mode list if detection fails
*/

#ifndef DISABLE_HAIKU

#include "fb_gfx_haiku.h"
#include "haiku_debug.h"

#include <Screen.h>

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Helper: check if mode already exists                                      */
/* ------------------------------------------------------------------------- */

static int fb_hHaikuModeExists(const int *modes, int count, int packed)
{
    int i;

    if (!modes)
        return 0;

    for (i = 0; i < count; ++i)
    {
        if (modes[i] == packed)
            return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
/* Helper: add a resolution to the list                                      */
/* ------------------------------------------------------------------------- */

static int fb_hHaikuAddMode(int *modes, int *count, int maxcount, int w, int h)
{
    int packed;

    if (!modes || !count)
        return 0;

    if (w <= 0 || h <= 0)
        return 0;

    packed = (w << 16) | (h & 0xFFFF);

    if (fb_hHaikuModeExists(modes, *count, packed))
        return 1;

    if (*count >= maxcount)
        return 0;

    modes[*count] = packed;
    (*count)++;

    HAIKU_DEBUG("Added mode %dx%d", w, h);

    return 1;
}

/* ------------------------------------------------------------------------- */
/* Mode enumeration                                                          */
/* ------------------------------------------------------------------------- */

int *fb_hHaikuFetchModes(int depth, int *size)
{
    display_mode *mode_list = NULL;
    uint32 mode_count = 0;

    int *modes = NULL;
    int count = 0;
    int capacity = 256;

    uint32 i;

    fb_hHaikuInitDebug();

    HAIKU_DEBUG("fb_hHaikuFetchModes(depth=%d)", depth);

    if (!size)
    {
        HAIKU_DEBUG("Mode enumeration failed: size pointer is NULL");
        return NULL;
    }

    *size = 0;

    modes = (int*)malloc(sizeof(int) * capacity);

    if (!modes)
    {
        HAIKU_DEBUG("Mode list allocation failed");
        return NULL;
    }

    memset(modes, 0, sizeof(int) * capacity);

    /* --------------------------------------------------------------------- */
    /* Real display modes from Haiku                                         */
    /* --------------------------------------------------------------------- */

    BScreen screen;

    if (screen.GetModeList(&mode_list, &mode_count) == B_OK && mode_list)
    {
        HAIKU_DEBUG("System reported %lu display modes",
                    (unsigned long)mode_count);

        for (i = 0; i < mode_count; ++i)
        {
            int w = (int)mode_list[i].timing.h_display;
            int h = (int)mode_list[i].timing.v_display;

            fb_hHaikuAddMode(modes, &count, capacity, w, h);
        }

        free(mode_list);
        mode_list = NULL;
    }
    else
    {
        HAIKU_DEBUG("Display mode query failed");
    }

    /* --------------------------------------------------------------------- */
    /* Desktop resolution                                                    */
    /* --------------------------------------------------------------------- */

    fb_hHaikuQueryDesktop();

    if (fb_haiku.desktop_width > 0 && fb_haiku.desktop_height > 0)
    {
        fb_hHaikuAddMode(
            modes,
            &count,
            capacity,
            fb_haiku.desktop_width,
            fb_haiku.desktop_height
        );
    }
    else
    {
        HAIKU_DEBUG("Desktop query returned no valid size");
    }

    /* --------------------------------------------------------------------- */
    /* Legacy FreeBASIC framebuffer modes                                    */
    /* --------------------------------------------------------------------- */

    fb_hHaikuAddMode(modes, &count, capacity, 320, 200);
    fb_hHaikuAddMode(modes, &count, capacity, 320, 240);
    fb_hHaikuAddMode(modes, &count, capacity, 400, 300);
    fb_hHaikuAddMode(modes, &count, capacity, 512, 384);
    fb_hHaikuAddMode(modes, &count, capacity, 640, 400);
    fb_hHaikuAddMode(modes, &count, capacity, 640, 480);
    fb_hHaikuAddMode(modes, &count, capacity, 800, 600);
    fb_hHaikuAddMode(modes, &count, capacity, 1024, 768);
    fb_hHaikuAddMode(modes, &count, capacity, 1280, 720);
    fb_hHaikuAddMode(modes, &count, capacity, 1280, 1024);
    fb_hHaikuAddMode(modes, &count, capacity, 1600, 900);
    fb_hHaikuAddMode(modes, &count, capacity, 1920, 1080);

    /* --------------------------------------------------------------------- */
    /* Fallback safety modes                                                 */
    /* --------------------------------------------------------------------- */

    if (count == 0)
    {
        HAIKU_DEBUG("No modes detected, using fallback list");

        fb_hHaikuAddMode(modes, &count, capacity, 320, 200);
        fb_hHaikuAddMode(modes, &count, capacity, 640, 480);
        fb_hHaikuAddMode(modes, &count, capacity, 800, 600);
    }

    *size = count;

    HAIKU_DEBUG("Returning %d modes", count);

    return modes;
}

#endif

/* end of haiku_modes.cpp */
