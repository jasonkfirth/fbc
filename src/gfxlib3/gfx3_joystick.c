/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_joystick.c

    Purpose:

        Provide the legacy joystick boundary on targets without a native
        polling implementation.

    Responsibilities:

        - initialize missing-device outputs consistently
        - report that native legacy joystick polling is unavailable

    This file intentionally does NOT contain:

        - platform joystick APIs
        - event-driven gamepad snapshots
        - graphics window management
*/

#include "gfx3_joystick.h"

int fb_gfx3_platform_joystick_has_native_polling(void)
{
	return FALSE;
}

int fb_gfx3_platform_joystick_get(int id, ssize_t *buttons, float *axis1,
	float *axis2, float *axis3, float *axis4, float *axis5, float *axis6,
	float *axis7, float *axis8)
{
	float *axes[] = { axis1, axis2, axis3, axis4, axis5, axis6, axis7,
		axis8 };
	size_t i;

	(void)id;
	if (buttons != NULL)
		*buttons = -1;
	for (i = 0; i < sizeof(axes) / sizeof(axes[0]); ++i) {
		if (axes[i] != NULL)
			*axes[i] = -1000.0f;
	}
	return FB_GFX3_UNSUPPORTED;
}

/* end of gfx3_joystick.c */
