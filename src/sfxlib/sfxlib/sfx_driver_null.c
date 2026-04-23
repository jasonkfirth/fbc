/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_null.c

    Purpose:

        Implement a null audio backend.

        The null driver accepts audio buffers but discards them.
        It allows the sound system to initialize successfully
        even when no real audio backend is available.

    Responsibilities:

        • provide a safe fallback audio driver
        • allow mixer and command testing without hardware
        • maintain compatibility with the driver interface

    This file intentionally does NOT contain:

        • real audio device interaction
        • platform-specific code
        • mixer logic
*/

#include "fb_sfx.h"
#include "fb_sfx_driver.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Driver state                                                              */
/* ------------------------------------------------------------------------- */

static int null_initialized = 0;


/* ------------------------------------------------------------------------- */
/* Driver init                                                               */
/* ------------------------------------------------------------------------- */

static int null_driver_init(int rate, int channels, int buffer_size, int flags)
{
    (void)rate;
    (void)channels;
    (void)buffer_size;
    (void)flags;

    null_initialized = 1;

    SFX_DEBUG("sfx_driver_null: initialized");

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Driver shutdown                                                           */
/* ------------------------------------------------------------------------- */

static void null_driver_shutdown(void)
{
    null_initialized = 0;

    SFX_DEBUG("sfx_driver_null: shutdown");
}


/* ------------------------------------------------------------------------- */
/* Driver write                                                              */
/* ------------------------------------------------------------------------- */

/*
    Accept audio samples but discard them.

    This allows the mixer and buffer pipeline to operate
    normally during testing.
*/

static int null_driver_write(const float *buffer, int frames)
{
    (void)buffer;
    (void)frames;

    if (!null_initialized)
        return -1;

    /* intentionally discard audio */

    return frames;
}


/* ------------------------------------------------------------------------- */
/* Driver status                                                             */
/* ------------------------------------------------------------------------- */

const FB_SFX_DRIVER __fb_sfxDriverNull =
{
    "null",
    0,
    null_driver_init,
    null_driver_shutdown,
    null_driver_write,
    NULL,
    NULL,
    NULL,
    NULL
};

const FB_SFX_DRIVER fb_sfxDriverNull =
{
    "null",
    0,
    null_driver_init,
    null_driver_shutdown,
    null_driver_write,
    NULL,
    NULL,
    NULL,
    NULL
};


/* end of sfx_driver_null.c */
