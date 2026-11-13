/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_nuttx.c

    Purpose:

        Register the NuttX sfxlib driver list.

    Responsibilities:

        - expose the normal sfxlib driver list symbol for TARGET_OS=nuttx
        - prefer the NuttX audio-framework PCM backend when present
        - retain the shared null driver as a safe fallback

    This file intentionally does NOT contain:

        - a DAC, I2S, or HDMI transport implementation
        - a background audio worker
        - capture or MIDI support
*/

#include "../fb_sfx_driver.h"

#include <stddef.h>

extern const FB_SFX_DRIVER fb_sfxDriverNuttXAudio;

const FB_SFX_DRIVER *fb_sfx_drivers_list[] =
{
    &fb_sfxDriverNuttXAudio,
    &fb_sfxDriverNull_internal,
    NULL
};

/* end of sfx_nuttx.c */
