/*
    FreeBASIC Darwin controller polling
    -----------------------------------

    File: gfx_xpad.c

    Purpose:

        Expose macOS GameController devices through GETXPAD.

    Responsibilities:

        - load the GameController framework when available
        - poll extended gamepads by index
        - map macOS controller inputs into the shared XPAD bitfield
        - normalize analog values to the ranges expected by GETXPAD

    This file intentionally does NOT contain:

        - keyboard or mouse event handling
        - window-system controller events
        - rumble, LED, or force-feedback output
*/

#include "../fb_gfx.h"

#ifdef HOST_DARWIN

#include <dlfcn.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdint.h>
#include <string.h>

#define XPAD_DARWIN_MAX_DEVICES 16
#define XPAD_TRIGGER_DIGITAL_THRESHOLD (30.0f / 255.0f)

/* ------------------------------------------------------------------------- */
/* Objective-C runtime helpers                                               */
/* ------------------------------------------------------------------------- */

static SEL xpad_darwin_sel(const char *name)
{
	return sel_registerName(name);
}

static id xpad_darwin_msg_id(id obj, const char *sel_name)
{
	return ((id (*)(id, SEL))objc_msgSend)(obj, xpad_darwin_sel(sel_name));
}

static id xpad_darwin_msg_id_ulong(id obj, const char *sel_name, unsigned long arg)
{
	return ((id (*)(id, SEL, unsigned long))objc_msgSend)(obj, xpad_darwin_sel(sel_name), arg);
}

static void xpad_darwin_msg_void_id(id obj, const char *sel_name, id arg)
{
	((void (*)(id, SEL, id))objc_msgSend)(obj, xpad_darwin_sel(sel_name), arg);
}

static unsigned long xpad_darwin_msg_ulong(id obj, const char *sel_name)
{
	return ((unsigned long (*)(id, SEL))objc_msgSend)(obj, xpad_darwin_sel(sel_name));
}

static BOOL xpad_darwin_msg_bool(id obj, const char *sel_name)
{
	return ((BOOL (*)(id, SEL))objc_msgSend)(obj, xpad_darwin_sel(sel_name));
}

static BOOL xpad_darwin_msg_bool_sel(id obj, const char *sel_name, SEL arg)
{
	return ((BOOL (*)(id, SEL, SEL))objc_msgSend)(obj, xpad_darwin_sel(sel_name), arg);
}

static float xpad_darwin_msg_float(id obj, const char *sel_name)
{
	return ((float (*)(id, SEL))objc_msgSend)(obj, xpad_darwin_sel(sel_name));
}

static BOOL xpad_darwin_responds_to(id obj, const char *sel_name)
{
	if (!obj)
		return FALSE;

	return xpad_darwin_msg_bool_sel(obj, "respondsToSelector:", xpad_darwin_sel(sel_name));
}

static id xpad_darwin_msg_id_if_responds(id obj, const char *sel_name)
{
	if (!xpad_darwin_responds_to(obj, sel_name))
		return nil;

	return xpad_darwin_msg_id(obj, sel_name);
}

/* ------------------------------------------------------------------------- */
/* Shared XPAD output handling                                               */
/* ------------------------------------------------------------------------- */

static void xpad_clear_outputs(ssize_t *buttons,
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

static float xpad_clamp_unit(float value)
{
	if (value < 0.0f)
		return 0.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

static float xpad_clamp_axis(float value)
{
	if (value < -1.0f)
		return -1.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

/* ------------------------------------------------------------------------- */
/* GameController framework access                                           */
/* ------------------------------------------------------------------------- */

static void *xpad_darwin_framework;
static Class xpad_darwin_controller_class;
static int xpad_darwin_load_tried;
static int xpad_darwin_discovery_started;
static int xpad_darwin_seen[XPAD_DARWIN_MAX_DEVICES];

static int xpad_darwin_load_framework(void)
{
	if (xpad_darwin_load_tried)
		return xpad_darwin_controller_class != Nil;

	xpad_darwin_load_tried = TRUE;

	/*
		GameController is used through the Objective-C runtime so older or
		reduced systems can report "missing" instead of failing to launch.
		The framework must still be loaded before GCController is visible.
	*/
	xpad_darwin_framework = dlopen(
		"/System/Library/Frameworks/GameController.framework/GameController",
		RTLD_LAZY | RTLD_LOCAL
	);
	if (!xpad_darwin_framework)
		return FALSE;

	xpad_darwin_controller_class = objc_getClass("GCController");
	if (xpad_darwin_controller_class == Nil)
		return FALSE;

	if (!xpad_darwin_discovery_started &&
		xpad_darwin_responds_to((id)xpad_darwin_controller_class, "startWirelessControllerDiscoveryWithCompletionHandler:")) {
		xpad_darwin_discovery_started = TRUE;
		xpad_darwin_msg_void_id(
			(id)xpad_darwin_controller_class,
			"startWirelessControllerDiscoveryWithCompletionHandler:",
			nil
		);
	}

	return TRUE;
}

static id xpad_darwin_get_controller(int pad_id)
{
	id controllers;
	unsigned long count;

	if (!xpad_darwin_load_framework())
		return nil;

	if (!xpad_darwin_responds_to((id)xpad_darwin_controller_class, "controllers"))
		return nil;

	controllers = xpad_darwin_msg_id((id)xpad_darwin_controller_class, "controllers");
	if (!controllers)
		return nil;

	count = xpad_darwin_msg_ulong(controllers, "count");
	if ((unsigned long)pad_id >= count)
		return nil;

	return xpad_darwin_msg_id_ulong(controllers, "objectAtIndex:", (unsigned long)pad_id);
}

/* ------------------------------------------------------------------------- */
/* GameController value mapping                                              */
/* ------------------------------------------------------------------------- */

static float xpad_darwin_button_value(id button)
{
	if (!button)
		return 0.0f;

	return xpad_clamp_unit(xpad_darwin_msg_float(button, "value"));
}

static int xpad_darwin_button_pressed(id button)
{
	if (!button)
		return FALSE;

	return xpad_darwin_msg_bool(button, "isPressed") != FALSE;
}

static float xpad_darwin_axis_value(id direction_pad, const char *axis_name)
{
	id axis;

	if (!direction_pad)
		return 0.0f;

	axis = xpad_darwin_msg_id(direction_pad, axis_name);
	if (!axis)
		return 0.0f;

	return xpad_clamp_axis(xpad_darwin_msg_float(axis, "value"));
}

static ssize_t xpad_darwin_dpad(id direction_pad)
{
	ssize_t dpad = 0;

	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(direction_pad, "up")))
		dpad |= XPAD_DPAD_UP;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(direction_pad, "right")))
		dpad |= XPAD_DPAD_RIGHT;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(direction_pad, "down")))
		dpad |= XPAD_DPAD_DOWN;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(direction_pad, "left")))
		dpad |= XPAD_DPAD_LEFT;

	return dpad;
}

static ssize_t xpad_darwin_buttons(id gamepad)
{
	ssize_t buttons = 0;
	id left_trigger;
	id right_trigger;

	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(gamepad, "buttonA")))
		buttons |= XPAD_BUTTON_A;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(gamepad, "buttonB")))
		buttons |= XPAD_BUTTON_B;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(gamepad, "buttonX")))
		buttons |= XPAD_BUTTON_X;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(gamepad, "buttonY")))
		buttons |= XPAD_BUTTON_Y;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(gamepad, "leftShoulder")))
		buttons |= XPAD_BUTTON_L1;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id(gamepad, "rightShoulder")))
		buttons |= XPAD_BUTTON_R1;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id_if_responds(gamepad, "leftThumbstickButton")))
		buttons |= XPAD_BUTTON_L3;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id_if_responds(gamepad, "rightThumbstickButton")))
		buttons |= XPAD_BUTTON_R3;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id_if_responds(gamepad, "buttonMenu")))
		buttons |= XPAD_BUTTON_START;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id_if_responds(gamepad, "buttonOptions")))
		buttons |= XPAD_BUTTON_SELECT;
	if (xpad_darwin_button_pressed(xpad_darwin_msg_id_if_responds(gamepad, "buttonHome")))
		buttons |= XPAD_BUTTON_GUIDE;

	left_trigger = xpad_darwin_msg_id(gamepad, "leftTrigger");
	right_trigger = xpad_darwin_msg_id(gamepad, "rightTrigger");

	if (xpad_darwin_button_value(left_trigger) > XPAD_TRIGGER_DIGITAL_THRESHOLD)
		buttons |= XPAD_BUTTON_L2;
	if (xpad_darwin_button_value(right_trigger) > XPAD_TRIGGER_DIGITAL_THRESHOLD)
		buttons |= XPAD_BUTTON_R2;

	return buttons;
}

static int xpad_darwin_get(int pad_id, ssize_t *buttons,
						   float *lstick_x, float *lstick_y,
						   float *rstick_x, float *rstick_y,
						   float *ltrigger, float *rtrigger,
						   ssize_t *dpad)
{
	id controller;
	id gamepad;
	id left_thumbstick;
	id right_thumbstick;
	id left_trigger;
	id right_trigger;

	if ((pad_id < 0) || (pad_id >= XPAD_DARWIN_MAX_DEVICES))
		return XPAD_STATUS_MISSING;

	controller = xpad_darwin_get_controller(pad_id);
	if (!controller)
		return xpad_darwin_seen[pad_id] ? XPAD_STATUS_DISCONNECTED : XPAD_STATUS_MISSING;

	gamepad = xpad_darwin_msg_id_if_responds(controller, "extendedGamepad");
	if (!gamepad)
		return xpad_darwin_seen[pad_id] ? XPAD_STATUS_DISCONNECTED : XPAD_STATUS_MISSING;

	xpad_darwin_seen[pad_id] = TRUE;

	left_thumbstick = xpad_darwin_msg_id(gamepad, "leftThumbstick");
	right_thumbstick = xpad_darwin_msg_id(gamepad, "rightThumbstick");
	left_trigger = xpad_darwin_msg_id(gamepad, "leftTrigger");
	right_trigger = xpad_darwin_msg_id(gamepad, "rightTrigger");

	if (buttons)
		*buttons = xpad_darwin_buttons(gamepad);
	if (lstick_x)
		*lstick_x = xpad_darwin_axis_value(left_thumbstick, "xAxis");
	if (lstick_y)
		*lstick_y = xpad_darwin_axis_value(left_thumbstick, "yAxis");
	if (rstick_x)
		*rstick_x = xpad_darwin_axis_value(right_thumbstick, "xAxis");
	if (rstick_y)
		*rstick_y = xpad_darwin_axis_value(right_thumbstick, "yAxis");
	if (ltrigger)
		*ltrigger = xpad_darwin_button_value(left_trigger);
	if (rtrigger)
		*rtrigger = xpad_darwin_button_value(right_trigger);
	if (dpad)
		*dpad = xpad_darwin_dpad(xpad_darwin_msg_id(gamepad, "dpad"));

	return XPAD_STATUS_CONNECTED;
}

FBCALL int fb_GfxGetXPad(int pad_id, ssize_t *buttons,
						 float *lstick_x, float *lstick_y,
						 float *rstick_x, float *rstick_y,
						 float *ltrigger, float *rtrigger,
						 ssize_t *dpad)
{
	int status;

	xpad_clear_outputs(buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);

	status = xpad_darwin_get(pad_id, buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);
	if (status != XPAD_STATUS_CONNECTED)
		xpad_clear_outputs(buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);

	return status;
}

#endif

/* end of gfx_xpad.c */
