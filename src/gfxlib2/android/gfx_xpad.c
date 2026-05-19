/*
    FreeBASIC gfxlib2 Android XPAD backend
    --------------------------------------

    File: gfx_xpad.c

    Purpose:

        Provide GETXPAD support from Android gamepad state collected by
        the NativeActivity input bridge.

    Responsibilities:

        - return Xbox-style button, trigger, stick, and d-pad state
        - keep disconnected/missing controller output values predictable
        - preserve the same public status constants used by other XPAD
          backends

    This file intentionally does NOT contain:

        - Android input queue ownership
        - joystick compatibility mapping
        - force feedback or rumble support
*/

#include "../fb_gfx.h"
#include "fb_gfx_android.h"

#define ANDROID_XPAD_MAX_DEVICES 16

static void android_xpad_clear_outputs(ssize_t *buttons,
									   float *lstick_x, float *lstick_y,
									   float *rstick_x, float *rstick_y,
									   float *ltrigger, float *rtrigger,
									   ssize_t *dpad)
{
	if (buttons)
		*buttons = 0;
	if (lstick_x)
		*lstick_x = 0.0f;
	if (lstick_y)
		*lstick_y = 0.0f;
	if (rstick_x)
		*rstick_x = 0.0f;
	if (rstick_y)
		*rstick_y = 0.0f;
	if (ltrigger)
		*ltrigger = 0.0f;
	if (rtrigger)
		*rtrigger = 0.0f;
	if (dpad)
		*dpad = 0;
}

FBCALL int fb_GfxGetXPad(int id, ssize_t *buttons,
						 float *lstick_x, float *lstick_y,
						 float *rstick_x, float *rstick_y,
						 float *ltrigger, float *rtrigger,
						 ssize_t *dpad)
{
	int status;

	FB_GRAPHICS_LOCK( );

	android_xpad_clear_outputs(buttons, lstick_x, lstick_y,
							   rstick_x, rstick_y,
							   ltrigger, rtrigger, dpad);

	if ((id < 0) || (id >= ANDROID_XPAD_MAX_DEVICES))
	{
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	status = fb_hAndroidGetXPad(id, buttons, lstick_x, lstick_y,
								rstick_x, rstick_y,
								ltrigger, rtrigger, dpad);
	if (status != XPAD_STATUS_CONNECTED)
		android_xpad_clear_outputs(buttons, lstick_x, lstick_y,
								   rstick_x, rstick_y,
								   ltrigger, rtrigger, dpad);

	FB_GRAPHICS_UNLOCK( );
	fb_ErrorSetNum(FB_RTERROR_OK);
	return status;
}

/* end of gfx_xpad.c */
