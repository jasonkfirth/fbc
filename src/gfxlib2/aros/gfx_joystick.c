/*
    FreeBASIC gfxlib2 support for AROS
    ----------------------------------

    File: gfx_joystick.c

    Purpose:

        Provide a deterministic GETJOYSTICK fallback on AROS.

    Responsibilities:

        - keep programs using GETJOYSTICK linkable
        - return the conventional missing-device values
        - report the unsupported controller through the runtime error API

    This file intentionally does NOT contain:

        - lowlevel.library controller access
        - USB HID parsing
        - keyboard-to-controller emulation
*/

#include "../fb_gfx.h"

FBCALL int fb_GfxGetJoystick(int id, ssize_t *buttons,
    float *a1, float *a2, float *a3, float *a4,
    float *a5, float *a6, float *a7, float *a8)
{
    (void)id;

    if (buttons != NULL)
        *buttons = -1;
    if (a1 != NULL) *a1 = -1000.0f;
    if (a2 != NULL) *a2 = -1000.0f;
    if (a3 != NULL) *a3 = -1000.0f;
    if (a4 != NULL) *a4 = -1000.0f;
    if (a5 != NULL) *a5 = -1000.0f;
    if (a6 != NULL) *a6 = -1000.0f;
    if (a7 != NULL) *a7 = -1000.0f;
    if (a8 != NULL) *a8 = -1000.0f;

    return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

/* end of gfx_joystick.c */
