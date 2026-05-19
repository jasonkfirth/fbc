/*
    FreeBASIC gfxlib2 Haiku controller polling
    ------------------------------------------

    File: gfx_xpad.c

    Purpose:

        Expose Haiku joystick devices through the GETXPAD API.

    Responsibilities:

        - provide the exported fb_GfxGetXPad symbol for Haiku builds
        - keep the C ABI boundary separate from the C++ BJoystick code
        - normalize the missing-device path to the GETXPAD contract

    This file intentionally does NOT contain:

        - BJoystick interaction
        - window or event handling
        - rumble support
*/

#ifndef DISABLE_HAIKU

#include "../fb_gfx.h"

extern int fb_hGfxHaikuGetXPad(
    int id,
    ssize_t *buttons,
    float *lstick_x,float *lstick_y,
    float *rstick_x,float *rstick_y,
    float *ltrigger,float *rtrigger,
    ssize_t *dpad
);

static void clear_outputs(
    ssize_t *buttons,
    float *lstick_x,float *lstick_y,
    float *rstick_x,float *rstick_y,
    float *ltrigger,float *rtrigger,
    ssize_t *dpad
)
{
    if (buttons) *buttons = 0;
    if (lstick_x) *lstick_x = 0.0f;
    if (lstick_y) *lstick_y = 0.0f;
    if (rstick_x) *rstick_x = 0.0f;
    if (rstick_y) *rstick_y = 0.0f;
    if (ltrigger) *ltrigger = 0.0f;
    if (rtrigger) *rtrigger = 0.0f;
    if (dpad) *dpad = 0;
}

FBCALL int fb_GfxGetXPad(
    int id,
    ssize_t *buttons,
    float *lstick_x,float *lstick_y,
    float *rstick_x,float *rstick_y,
    float *ltrigger,float *rtrigger,
    ssize_t *dpad
)
{
    int status;

    FB_GRAPHICS_LOCK();

    clear_outputs(buttons,lstick_x,lstick_y,rstick_x,rstick_y,ltrigger,rtrigger,dpad);

    if (id < 0 || id >= 16)
    {
        FB_GRAPHICS_UNLOCK();
        return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
    }

    status = fb_hGfxHaikuGetXPad(
        id,
        buttons,
        lstick_x,lstick_y,
        rstick_x,rstick_y,
        ltrigger,rtrigger,
        dpad
    );

    if (status != XPAD_STATUS_CONNECTED)
        clear_outputs(buttons,lstick_x,lstick_y,rstick_x,rstick_y,ltrigger,rtrigger,dpad);

    fb_ErrorSetNum(FB_RTERROR_OK);
    FB_GRAPHICS_UNLOCK();
    return status;
}

#endif

/* end of gfx_xpad.c */
