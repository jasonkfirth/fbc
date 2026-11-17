/*
    FreeBASIC gfxlib2 OpenBSD joystick backend
    ------------------------------------------

    File: gfx_joystick.h

    Purpose:

        Declare the shared joystick state interface used by the OpenBSD
        GETJOYSTICK and GETXPAD backends.

    Responsibilities:

        - keep the HID parser and XPAD mapper on one function signature

    This file intentionally does NOT contain:

        - HID report parsing or device state
        - public FreeBASIC graphics API declarations
*/

#ifndef FB_GFX_OPENBSD_JOYSTICK_H
#define FB_GFX_OPENBSD_JOYSTICK_H

#include <sys/types.h>

int fb_hGfxOpenbsdGetJoystickState(int id,
                                   ssize_t *buttons,
                                   float *a1, float *a2,
                                   float *a3, float *a4,
                                   float *a5, float *a6,
                                   float *a7, float *a8,
                                   ssize_t *dpad);

#endif

/* end of gfx_joystick.h */
