/*
    FreeBASIC gfxlib2 OpenBSD joystick backend
    ------------------------------------------

    File: gfx_joystick.c

    Purpose:

        Provide GETJOYSTICK support through OpenBSD's USB joystick HID
        devices.

    Responsibilities:

        - discover /dev/ujoy/N and /dev/uhidN devices
        - parse HID input descriptors with libusbhid
        - poll reports without blocking the BASIC program
        - normalize axes to FreeBASIC's -1.0 through 1.0 range

    This file intentionally does NOT contain:

        - Linux joydev compatibility code
        - Xbox semantic controller mapping
        - force feedback or rumble support

    Platform notes:

        OpenBSD's ujoy(4) exposes USB game controllers as HID devices under
        /dev/ujoy/N.  The device is compatible with read(2) and the descriptor
        parsing routines from usbhid(3), so this backend stores the relevant
        input HID items once and then extracts values from each report.
*/

#include "../fb_gfx.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <usbhid.h>

#define FB_OPENBSD_JOY_MAX_DEVICES 16
#define FB_OPENBSD_JOY_MAX_AXES 8
#define FB_OPENBSD_JOY_MAX_BUTTONS 32
#define FB_OPENBSD_JOY_REPORT_BYTES 1024

#define FB_HID_USAGE_PAGE_GENERIC_DESKTOP 0x0001u
#define FB_HID_USAGE_PAGE_BUTTON          0x0009u

#define FB_HID_USAGE_X                    0x0030u
#define FB_HID_USAGE_Y                    0x0031u
#define FB_HID_USAGE_Z                    0x0032u
#define FB_HID_USAGE_RX                   0x0033u
#define FB_HID_USAGE_RY                   0x0034u
#define FB_HID_USAGE_RZ                   0x0035u
#define FB_HID_USAGE_SLIDER               0x0036u
#define FB_HID_USAGE_DIAL                 0x0037u
#define FB_HID_USAGE_WHEEL                0x0038u
#define FB_HID_USAGE_HAT_SWITCH           0x0039u

typedef struct FB_OPENBSD_JOY_ {
	int fd;
	report_desc_t report_desc;
	int axis_count;
	int button_count;
	int has_hat;
	hid_item_t axis_item[FB_OPENBSD_JOY_MAX_AXES];
	hid_item_t button_item[FB_OPENBSD_JOY_MAX_BUTTONS];
	hid_item_t hat_item;
	float axis[FB_OPENBSD_JOY_MAX_AXES];
	uint32_t buttons;
	ssize_t dpad;
} FB_OPENBSD_JOY;

static FB_OPENBSD_JOY fb_openbsd_joy[FB_OPENBSD_JOY_MAX_DEVICES];
static int fb_openbsd_joy_inited;

static void fb_openbsd_joy_clear_outputs(ssize_t *buttons,
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

static void fb_openbsd_joy_close(FB_OPENBSD_JOY *joy)
{
	if (joy->fd >= 0)
		close(joy->fd);

	if (joy->report_desc)
		hid_dispose_report_desc(joy->report_desc);

	joy->fd = -1;
	joy->report_desc = NULL;
	joy->axis_count = 0;
	joy->button_count = 0;
	joy->has_hat = 0;
	joy->buttons = 0;
	joy->dpad = 0;
}

static int fb_openbsd_joy_axis_usage(unsigned int usage)
{
	if (HID_PAGE(usage) != FB_HID_USAGE_PAGE_GENERIC_DESKTOP)
		return FALSE;

	switch (HID_USAGE(usage)) {
	case FB_HID_USAGE_X:
	case FB_HID_USAGE_Y:
	case FB_HID_USAGE_Z:
	case FB_HID_USAGE_RX:
	case FB_HID_USAGE_RY:
	case FB_HID_USAGE_RZ:
	case FB_HID_USAGE_SLIDER:
	case FB_HID_USAGE_DIAL:
	case FB_HID_USAGE_WHEEL:
		return TRUE;
	default:
		return FALSE;
	}
}

static int fb_openbsd_joy_button_usage(unsigned int usage)
{
	unsigned int button;

	if (HID_PAGE(usage) != FB_HID_USAGE_PAGE_BUTTON)
		return FALSE;

	button = HID_USAGE(usage);
	return (button >= 1) && (button <= FB_OPENBSD_JOY_MAX_BUTTONS);
}

static int fb_openbsd_joy_hat_usage(unsigned int usage)
{
	return (HID_PAGE(usage) == FB_HID_USAGE_PAGE_GENERIC_DESKTOP) &&
	       (HID_USAGE(usage) == FB_HID_USAGE_HAT_SWITCH);
}

static float fb_openbsd_joy_normalize_axis(int value, const hid_item_t *item)
{
	int minimum;
	int maximum;

	minimum = item->logical_minimum;
	maximum = item->logical_maximum;

	if (maximum <= minimum)
		return 0.0f;

	if (value <= minimum)
		return -1.0f;
	if (value >= maximum)
		return 1.0f;

	return (((float)(value - minimum) * 2.0f) / (float)(maximum - minimum)) - 1.0f;
}

static ssize_t fb_openbsd_joy_dpad_from_hat(int value, const hid_item_t *item)
{
	int hat;

	if (item->logical_minimum == 1 && item->logical_maximum == 8)
		hat = value - 1;
	else
		hat = value;

	if (hat < 0 || hat > 7)
		return 0;

	switch (hat) {
	case 0: return XPAD_DPAD_UP;
	case 1: return XPAD_DPAD_UP | XPAD_DPAD_RIGHT;
	case 2: return XPAD_DPAD_RIGHT;
	case 3: return XPAD_DPAD_RIGHT | XPAD_DPAD_DOWN;
	case 4: return XPAD_DPAD_DOWN;
	case 5: return XPAD_DPAD_DOWN | XPAD_DPAD_LEFT;
	case 6: return XPAD_DPAD_LEFT;
	case 7: return XPAD_DPAD_LEFT | XPAD_DPAD_UP;
	default: return 0;
	}
}

static int fb_openbsd_joy_scan_items(FB_OPENBSD_JOY *joy)
{
	hid_data_t parser;
	hid_item_t item;

	parser = hid_start_parse(joy->report_desc, 1 << hid_input, -1);
	if (!parser)
		return FALSE;

	while (hid_get_item(parser, &item) > 0) {
		if (item.kind != hid_input)
			continue;

		if (fb_openbsd_joy_axis_usage(item.usage)) {
			if (joy->axis_count < FB_OPENBSD_JOY_MAX_AXES) {
				joy->axis_item[joy->axis_count] = item;
				joy->axis[joy->axis_count] = 0.0f;
				++joy->axis_count;
			}
			continue;
		}

		if (fb_openbsd_joy_hat_usage(item.usage)) {
			if (!joy->has_hat) {
				joy->hat_item = item;
				joy->has_hat = TRUE;
			}
			continue;
		}

		if (fb_openbsd_joy_button_usage(item.usage)) {
			if (joy->button_count < FB_OPENBSD_JOY_MAX_BUTTONS) {
				joy->button_item[joy->button_count] = item;
				++joy->button_count;
			}
			continue;
		}
	}

	hid_end_parse(parser);
	return (joy->axis_count > 0) || (joy->button_count > 0);
}

static int fb_openbsd_joy_open_path(FB_OPENBSD_JOY *joy, const char *prefix, int index)
{
	char device_name[32];

	if (snprintf(device_name, sizeof(device_name), "%s%d", prefix, index) >=
		(int)sizeof(device_name))
		return FALSE;

	joy->fd = open(device_name, O_RDONLY | O_NONBLOCK);
	if (joy->fd < 0)
		return FALSE;

	joy->report_desc = hid_get_report_desc(joy->fd);
	if (!joy->report_desc || !fb_openbsd_joy_scan_items(joy)) {
		fb_openbsd_joy_close(joy);
		return FALSE;
	}

	return TRUE;
}

static void fb_openbsd_joy_init(void)
{
	static const char *const device_path[] = {
		"/dev/ujoy/",
		"/dev/uhid",
		NULL
	};
	FB_OPENBSD_JOY *joy;
	int count;
	int i;
	int j;

	if (fb_openbsd_joy_inited)
		return;

	fb_hMemSet(fb_openbsd_joy, 0, sizeof(fb_openbsd_joy));
	for (i = 0; i < FB_OPENBSD_JOY_MAX_DEVICES; ++i)
		fb_openbsd_joy[i].fd = -1;

	count = 0;
	for (i = 0; device_path[i] && (count < FB_OPENBSD_JOY_MAX_DEVICES); ++i) {
		for (j = 0; (j < FB_OPENBSD_JOY_MAX_DEVICES) &&
					(count < FB_OPENBSD_JOY_MAX_DEVICES); ++j) {
			joy = &fb_openbsd_joy[count];
			if (!fb_openbsd_joy_open_path(joy, device_path[i], j))
				continue;

			++count;
		}
	}

	fb_openbsd_joy_inited = TRUE;
}

static void fb_openbsd_joy_update_report(FB_OPENBSD_JOY *joy,
										 const unsigned char *report)
{
	int i;

	for (i = 0; i < joy->axis_count; ++i) {
		int value;

		value = hid_get_data((void *)report, &joy->axis_item[i]);
		joy->axis[i] = fb_openbsd_joy_normalize_axis(value, &joy->axis_item[i]);
	}

	joy->buttons = 0;
	for (i = 0; i < joy->button_count; ++i) {
		if (hid_get_data((void *)report, &joy->button_item[i]))
			joy->buttons |= (1u << i);
	}

	if (joy->has_hat)
		joy->dpad = fb_openbsd_joy_dpad_from_hat(
			hid_get_data((void *)report, &joy->hat_item),
			&joy->hat_item);
}

static int fb_openbsd_joy_poll(FB_OPENBSD_JOY *joy)
{
	unsigned char report[FB_OPENBSD_JOY_REPORT_BYTES];
	ssize_t bytes;

	for (;;) {
		fb_hMemSet(report, 0, sizeof(report));
		bytes = read(joy->fd, report, sizeof(report));
		if (bytes <= 0)
			break;
		fb_openbsd_joy_update_report(joy, report);
	}

	if ((bytes < 0) &&
		((errno == ENODEV) || (errno == ENXIO) || (errno == EIO))) {
		fb_openbsd_joy_close(joy);
		return FALSE;
	}

	return TRUE;
}

int fb_hGfxOpenbsdGetJoystickState(int id,
								   ssize_t *buttons,
								   float *a1, float *a2,
								   float *a3, float *a4,
								   float *a5, float *a6,
								   float *a7, float *a8,
								   ssize_t *dpad)
{
	FB_OPENBSD_JOY *joy;

	fb_openbsd_joy_clear_outputs(buttons, a1, a2, a3, a4, a5, a6, a7, a8);
	if (dpad)
		*dpad = 0;

	if ((id < 0) || (id >= FB_OPENBSD_JOY_MAX_DEVICES))
		return FALSE;

	fb_openbsd_joy_init();

	joy = &fb_openbsd_joy[id];
	if (joy->fd < 0)
		return FALSE;

	if (!fb_openbsd_joy_poll(joy))
		return FALSE;

	if (a1)
		*a1 = joy->axis[0];
	if (a2)
		*a2 = joy->axis[1];
	if (a3)
		*a3 = joy->axis[2];
	if (a4)
		*a4 = joy->axis[3];
	if (a5)
		*a5 = joy->axis[4];
	if (a6)
		*a6 = joy->axis[5];
	if (a7)
		*a7 = joy->axis[6];
	if (a8)
		*a8 = joy->axis[7];
	if (buttons)
		*buttons = (ssize_t)joy->buttons;
	if (dpad)
		*dpad = joy->dpad;

	return TRUE;
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

	connected = fb_hGfxOpenbsdGetJoystickState(id, buttons,
											   a1, a2, a3, a4,
											   a5, a6, a7, a8,
											   NULL);
	if (!connected) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	FB_GRAPHICS_UNLOCK( );
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of gfx_joystick.c */
