/*
    FreeBASIC gfxlib2 Darwin joystick compatibility
    ------------------------------------------------

    File: gfx_joystick.c

    Purpose:

        Provide the exported GETJOYSTICK backend expected by gfxlib2
        and by the QB STICK/STRIG compatibility functions on macOS.

    Responsibilities:

        - expose the fb_GfxGetJoystick symbol for Darwin builds
        - map GameController-backed GETXPAD state to the older joystick ABI
        - preserve the traditional GETJOYSTICK "missing device" defaults

    This file intentionally does NOT contain:

        - direct HID device parsing
        - Objective-C GameController access
        - force feedback or controller output
*/

#include "../fb_gfx.h"

#ifdef HOST_DARWIN

/* ------------------------------------------------------------------------- */
/* Output handling                                                           */
/* ------------------------------------------------------------------------- */

static void darwin_joystick_clear_outputs(ssize_t *buttons,
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

static float darwin_joystick_dpad_axis(ssize_t dpad, ssize_t negative, ssize_t positive)
{
	if (dpad & negative)
		return -1.0f;
	if (dpad & positive)
		return 1.0f;

	return 0.0f;
}

/* ------------------------------------------------------------------------- */
/* GETJOYSTICK                                                               */
/* ------------------------------------------------------------------------- */

FBCALL int fb_GfxGetJoystick(int id,
							 ssize_t *buttons,
							 float *a1, float *a2,
							 float *a3, float *a4,
							 float *a5, float *a6,
							 float *a7, float *a8)
{
	ssize_t xpad_buttons;
	ssize_t xpad_dpad;
	float left_x;
	float left_y;
	float right_x;
	float right_y;
	float left_trigger;
	float right_trigger;
	int status;

	darwin_joystick_clear_outputs(buttons, a1, a2, a3, a4, a5, a6, a7, a8);

	if (id < 0)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

	xpad_buttons = 0;
	xpad_dpad = 0;
	left_x = 0.0f;
	left_y = 0.0f;
	right_x = 0.0f;
	right_y = 0.0f;
	left_trigger = 0.0f;
	right_trigger = 0.0f;

	/*
	    macOS exposes modern controllers through GameController.framework.
	    The Darwin GETXPAD backend owns that platform-specific polling code,
	    so GETJOYSTICK reuses it instead of growing a second HID stack.
	*/
	status = fb_GfxGetXPad(id, &xpad_buttons, &left_x, &left_y,
						   &right_x, &right_y, &left_trigger,
						   &right_trigger, &xpad_dpad);
	if (status != XPAD_STATUS_CONNECTED)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

	if (buttons)
		*buttons = xpad_buttons;
	if (a1)
		*a1 = left_x;
	if (a2)
		*a2 = left_y;
	if (a3)
		*a3 = right_x;
	if (a4)
		*a4 = right_y;
	if (a5)
		*a5 = left_trigger;
	if (a6)
		*a6 = right_trigger;
	if (a7)
		*a7 = darwin_joystick_dpad_axis(xpad_dpad, XPAD_DPAD_LEFT, XPAD_DPAD_RIGHT);
	if (a8)
		*a8 = darwin_joystick_dpad_axis(xpad_dpad, XPAD_DPAD_UP, XPAD_DPAD_DOWN);

	return fb_ErrorSetNum(FB_RTERROR_OK);
}

#endif

/* end of gfx_joystick.c */
