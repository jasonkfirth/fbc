/*
    FreeBASIC gfxlib2 Android joystick backend
    ------------------------------------------

    File: gfx_joystick.c

    Purpose:

        Provide GETJOYSTICK support from Android gamepad state collected
        by the NativeActivity input bridge.

    Responsibilities:

        - return the latest normalized Android controller axes
        - return the latest Android controller button bitfield
        - keep the public GETJOYSTICK error/default behavior stable when
          no controller has been seen

    This file intentionally does NOT contain:

        - Android input queue ownership
        - touchscreen or keyboard event handling
        - force feedback or rumble support
*/

#include "../fb_gfx.h"
#include "fb_gfx_android.h"

static void android_joystick_clear_outputs(ssize_t *buttons,
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
	int connected;

	FB_GRAPHICS_LOCK( );

	android_joystick_clear_outputs(buttons, a1, a2, a3, a4, a5, a6, a7, a8);
	connected = fb_hAndroidGetJoystick(id, buttons, a1, a2, a3, a4, a5, a6, a7, a8);
	if (!connected)
	{
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	FB_GRAPHICS_UNLOCK( );
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of gfx_joystick.c */
