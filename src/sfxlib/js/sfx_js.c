/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    JavaScript/Emscripten target driver list.

    The JS target currently exposes the generic null driver so sfxlib programs
    can link and run without a browser audio backend. A future WebAudio driver
    can be added ahead of this fallback.
*/

#include "../fb_sfx.h"

#include <stddef.h>

extern const FB_SFX_DRIVER __fb_sfxDriverNull;
extern const FB_SFX_DRIVER fb_sfxDriverWebAudio;

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverWebAudio,
    &__fb_sfxDriverNull,
    NULL
};

/* end of sfx_js.c */
