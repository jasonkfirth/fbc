/*
    FreeBASIC gfxlib2 support for RISC OS
    -------------------------------------

    File: gfx_joystick.c

    Purpose:

        Provide the GETJOYSTICK entry point for the RISC OS gfxlib2 backend.

    Responsibilities:

        - keep programs using GETJOYSTICK linkable on RISC OS
        - return the traditional missing-device values
        - report unsupported controller access through the runtime error API

    This file intentionally does NOT contain:

        - USB HID parsing
        - legacy analogue joystick SWIs
        - keyboard-to-controller emulation
        - graphics input polling
*/

#include "../fb_gfx.h"

static void riscos_joystick_clear_outputs(ssize_t *buttons,
    float *a1, float *a2, float *a3, float *a4,
    float *a5, float *a6, float *a7, float *a8)
{
    if (buttons != NULL)
        *buttons = -1;
    if (a1 != NULL)
        *a1 = -1000.0f;
    if (a2 != NULL)
        *a2 = -1000.0f;
    if (a3 != NULL)
        *a3 = -1000.0f;
    if (a4 != NULL)
        *a4 = -1000.0f;
    if (a5 != NULL)
        *a5 = -1000.0f;
    if (a6 != NULL)
        *a6 = -1000.0f;
    if (a7 != NULL)
        *a7 = -1000.0f;
    if (a8 != NULL)
        *a8 = -1000.0f;
}

FBCALL int fb_GfxGetJoystick(int id, ssize_t *buttons,
    float *a1, float *a2, float *a3, float *a4,
    float *a5, float *a6, float *a7, float *a8)
{
    (void)id;

    FB_GRAPHICS_LOCK();
    riscos_joystick_clear_outputs(buttons, a1, a2, a3, a4,
        a5, a6, a7, a8);
    FB_GRAPHICS_UNLOCK();

    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

/* end of gfx_joystick.c */
