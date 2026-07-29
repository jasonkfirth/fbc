/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_qb_input.c

    Purpose:

        Preserve QuickBASIC STICK and STRIG behavior above gfxlib3's portable
        joystick query.

    Responsibilities:

        - latch two joystick positions when STICK(0) is called
        - retain button presses for the even STRIG queries
        - report bounded zero state when a joystick is unavailable

    This file intentionally does NOT contain:

        - native controller discovery or polling
        - Xbox-style controller mapping
        - keyboard, mouse, or touch input
*/

#include "gfx3_api_internal.h"

FBCALL int fb_GfxGetJoystick(int id, ssize_t *buttons, float *axis1,
	float *axis2, float *axis3, float *axis4, float *axis5, float *axis6,
	float *axis7, float *axis8);

static int stick_positions[4];
static int stick_buttons[2];

FBCALL int fb_GfxStickQB(int index)
{
	int result = 0;

	FB_GRAPHICS_LOCK();
	if ((index >= 0) && (index <= 3)) {
		if (index == 0) {
			int joystick;

			for (joystick = 0; joystick < 2; joystick++) {
				float x;
				float y;
				float unused;
				ssize_t buttons;

				if (fb_GfxGetJoystick(joystick, &buttons, &x, &y,
				    &unused, &unused, &unused, &unused, &unused,
				    &unused) == FB_RTERROR_OK) {
					stick_positions[joystick * 2] =
						(int)(x * 100.0f + 101.0f);
					stick_positions[joystick * 2 + 1] =
						(int)(y * 100.0f + 101.0f);
					stick_buttons[joystick] = (int)buttons;
				} else {
					stick_positions[joystick * 2] = 0;
					stick_positions[joystick * 2 + 1] = 0;
					stick_buttons[joystick] = 0;
				}
			}
		}
		result = stick_positions[index];
	} else {
		fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	FB_GRAPHICS_UNLOCK();
	return result;
}

FBCALL int fb_GfxStrigQB(int index)
{
	int result = 0;

	FB_GRAPHICS_LOCK();
	if ((index >= 0) && (index <= 7)) {
		int joystick = (index >> 1) & 1;
		int button_mask = (index >> 2) + 1;

		if ((index & 1) != 0) {
			float unused;
			ssize_t buttons;

			if (fb_GfxGetJoystick(joystick, &buttons, &unused,
			    &unused, &unused, &unused, &unused, &unused, &unused,
			    &unused) == FB_RTERROR_OK) {
				stick_buttons[joystick] |= (int)buttons;
				result = ((int)buttons & button_mask) ? -1 : 0;
			} else {
				fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
			}
		} else {
			result = (stick_buttons[joystick] & button_mask) ? -1 : 0;
		}
	} else {
		fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	FB_GRAPHICS_UNLOCK();
	return result;
}

/* end of gfx3_qb_input.c */
