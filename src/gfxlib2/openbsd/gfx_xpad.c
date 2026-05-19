/*
    FreeBASIC gfxlib2 OpenBSD XPAD backend
    --------------------------------------

    File: gfx_xpad.c

    Purpose:

        Provide GETXPAD support from the OpenBSD ujoy HID backend.

    Responsibilities:

        - map the OpenBSD joystick state to the stable XPAD button layout
        - expose left/right sticks, triggers, and d-pad values when present
        - preserve predictable zero outputs for missing controllers

    This file intentionally does NOT contain:

        - HID descriptor parsing
        - Linux joydev compatibility code
        - force feedback or rumble support
*/

#include "../fb_gfx.h"

#define OPENBSD_XPAD_MAX_DEVICES 16
#define OPENBSD_XPAD_TRIGGER_BUTTON_THRESHOLD 0.20f

extern int fb_hGfxOpenbsdGetJoystickState(int id,
										  ssize_t *buttons,
										  float *a1, float *a2,
										  float *a3, float *a4,
										  float *a5, float *a6,
										  float *a7, float *a8,
										  ssize_t *dpad);

static void openbsd_xpad_clear_outputs(ssize_t *buttons,
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

static float openbsd_xpad_trigger(float axis)
{
	if (axis <= -100.0f)
		return 0.0f;

	axis = (axis + 1.0f) * 0.5f;
	if (axis < 0.0f)
		return 0.0f;
	if (axis > 1.0f)
		return 1.0f;
	return axis;
}

static ssize_t openbsd_xpad_buttons(ssize_t joystick_buttons,
									float left_trigger,
									float right_trigger)
{
	ssize_t buttons = 0;

	if (joystick_buttons & (1 << 0))
		buttons |= XPAD_BUTTON_A;
	if (joystick_buttons & (1 << 1))
		buttons |= XPAD_BUTTON_B;
	if (joystick_buttons & (1 << 2))
		buttons |= XPAD_BUTTON_X;
	if (joystick_buttons & (1 << 3))
		buttons |= XPAD_BUTTON_Y;
	if (joystick_buttons & (1 << 4))
		buttons |= XPAD_BUTTON_L1;
	if (joystick_buttons & (1 << 5))
		buttons |= XPAD_BUTTON_R1;
	if (joystick_buttons & (1 << 6))
		buttons |= XPAD_BUTTON_SELECT;
	if (joystick_buttons & (1 << 7))
		buttons |= XPAD_BUTTON_START;
	if (joystick_buttons & (1 << 8))
		buttons |= XPAD_BUTTON_GUIDE;
	if (joystick_buttons & (1 << 9))
		buttons |= XPAD_BUTTON_L3;
	if (joystick_buttons & (1 << 10))
		buttons |= XPAD_BUTTON_R3;
	if (left_trigger > OPENBSD_XPAD_TRIGGER_BUTTON_THRESHOLD)
		buttons |= XPAD_BUTTON_L2;
	if (right_trigger > OPENBSD_XPAD_TRIGGER_BUTTON_THRESHOLD)
		buttons |= XPAD_BUTTON_R2;

	return buttons;
}

FBCALL int fb_GfxGetXPad(int id, ssize_t *buttons,
						 float *lstick_x, float *lstick_y,
						 float *rstick_x, float *rstick_y,
						 float *ltrigger, float *rtrigger,
						 ssize_t *dpad)
{
	ssize_t joystick_buttons;
	ssize_t joystick_dpad;
	float a1, a2, a3, a4, a5, a6, a7, a8;
	float lt;
	float rt;
	int connected;

	FB_GRAPHICS_LOCK( );

	openbsd_xpad_clear_outputs(buttons, lstick_x, lstick_y,
							   rstick_x, rstick_y,
							   ltrigger, rtrigger, dpad);

	if ((id < 0) || (id >= OPENBSD_XPAD_MAX_DEVICES))
	{
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	connected = fb_hGfxOpenbsdGetJoystickState(id,
											   &joystick_buttons,
											   &a1, &a2, &a3, &a4,
											   &a5, &a6, &a7, &a8,
											   &joystick_dpad);
	if (!connected)
	{
		FB_GRAPHICS_UNLOCK( );
		fb_ErrorSetNum(FB_RTERROR_OK);
		return XPAD_STATUS_MISSING;
	}

	lt = openbsd_xpad_trigger(a5);
	rt = openbsd_xpad_trigger(a6);

	if (buttons)
		*buttons = openbsd_xpad_buttons(joystick_buttons, lt, rt);
	if (lstick_x)
		*lstick_x = a1;
	if (lstick_y)
		*lstick_y = a2;
	if (rstick_x)
		*rstick_x = a3;
	if (rstick_y)
		*rstick_y = a4;
	if (ltrigger)
		*ltrigger = lt;
	if (rtrigger)
		*rtrigger = rt;
	if (dpad)
		*dpad = joystick_dpad;

	(void)a7;
	(void)a8;

	FB_GRAPHICS_UNLOCK( );
	fb_ErrorSetNum(FB_RTERROR_OK);
	return XPAD_STATUS_CONNECTED;
}

/* end of gfx_xpad.c */
