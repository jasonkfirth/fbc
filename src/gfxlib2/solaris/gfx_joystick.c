/*
    FreeBASIC gfxlib2 Solaris joystick fallback
    -------------------------------------------

    File: gfx_joystick.c

    Purpose:

        Provide the GETJOYSTICK entry point on Solaris/illumos builds
        where gfxlib has no native joystick backend.

    Responsibilities:

        - keep programs using GETJOYSTICK linkable
        - return the traditional missing-device values
        - report unsupported joystick access as an illegal function call

    This file intentionally does NOT contain:

        - HID descriptor parsing
        - Linux joydev compatibility
        - controller hotplug support
*/

#include "../fb_gfx.h"

static void solaris_joystick_clear_outputs(ssize_t *buttons,
                                           float *a1, float *a2,
                                           float *a3, float *a4,
                                           float *a5, float *a6,
                                           float *a7, float *a8)
{
	if (buttons)
		*buttons = -1;
	if (a1)
		*a1 = -1000.0f;
	if (a2)
		*a2 = -1000.0f;
	if (a3)
		*a3 = -1000.0f;
	if (a4)
		*a4 = -1000.0f;
	if (a5)
		*a5 = -1000.0f;
	if (a6)
		*a6 = -1000.0f;
	if (a7)
		*a7 = -1000.0f;
	if (a8)
		*a8 = -1000.0f;
}

FBCALL int fb_GfxGetJoystick(int id,
                             ssize_t *buttons,
                             float *a1, float *a2,
                             float *a3, float *a4,
                             float *a5, float *a6,
                             float *a7, float *a8)
{
	(void)id;

	FB_GRAPHICS_LOCK( );
	solaris_joystick_clear_outputs(buttons, a1, a2, a3, a4, a5, a6, a7, a8);
	FB_GRAPHICS_UNLOCK( );

	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

/* end of gfx_joystick.c */
