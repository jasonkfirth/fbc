/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_joystick.h

    Purpose:

        Define the platform boundary for legacy GETJOYSTICK polling.

    Responsibilities:

        - preserve the gfxlib2 WinMM joystick contract where it exists
        - keep generic joystick polling separate from XInput GETXPAD state

    This file intentionally does NOT contain:

        - WinMM, XInput, Android, or other platform declarations
        - FreeBASIC public API entry points
        - render-thread input snapshots
*/

#ifndef __FB_GFX3_JOYSTICK_H__
#define __FB_GFX3_JOYSTICK_H__

#include "fb_gfx3.h"

int fb_gfx3_platform_joystick_get(int id, ssize_t *buttons, float *axis1,
	float *axis2, float *axis3, float *axis4, float *axis5, float *axis6,
	float *axis7, float *axis8);
int fb_gfx3_platform_joystick_has_native_polling(void);

#endif

/* end of gfx3_joystick.h */
