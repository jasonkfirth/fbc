/*
    FreeBASIC gfxlib2 Unix joydev backend
    -------------------------------------

    File: gfx_joydev_common.inc

    Purpose:

        Provide GETJOYSTICK support through the Linux joydev-compatible
        character-device interface.

    Responsibilities:

        • discover /dev/input/jsN and /dev/jsN devices
        • poll queued axis and button events without blocking
        • normalize axis data to FreeBASIC's -1.0 through 1.0 range
        • report missing devices using the traditional GETJOYSTICK failure
          values

    This file intentionally does NOT contain:

        • HID descriptor parsing
        • evdev event-device parsing
        • Xbox semantic button mapping
        • force feedback or rumble support

    Platform notes:

        Linux provides this API through joydev.  FreeBSD systems can expose the
        same API through webcamd as /dev/input/jsN.  Keeping this backend in one
        include keeps the ABI assumptions documented in one place while allowing
        each target directory to decide whether the interface is appropriate.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define FB_JOYDEV_MAX_DEVICES 16
#define FB_JOYDEV_MAX_AXES 8
#define FB_JOYDEV_MAX_BUTTONS 32

#define FB_JOYDEV_EVENT_BUTTON 0x01
#define FB_JOYDEV_EVENT_AXIS   0x02
#define FB_JOYDEV_EVENT_INIT   0x80

#define FB_JOYDEV_GET_VERSION _IOR('j', 0x01, unsigned int)

typedef struct FB_JOYDEV_EVENT_ {
	unsigned int timestamp;
	short value;
	unsigned char type;
	unsigned char number;
} FB_JOYDEV_EVENT;

typedef struct FB_JOYDEV_DEVICE_ {
	int fd;
	float axis[FB_JOYDEV_MAX_AXES];
	uint32_t buttons;
} FB_JOYDEV_DEVICE;

static FB_JOYDEV_DEVICE fb_joydev[FB_JOYDEV_MAX_DEVICES];
static int fb_joydev_inited;

static void fb_joydev_clear_outputs(ssize_t *buttons,
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

static float fb_joydev_normalize_axis(short value)
{
	if (value <= -32768)
		return -1.0f;
	if (value >= 32767)
		return 1.0f;
	return (float)value / 32767.0f;
}

static void fb_joydev_close(FB_JOYDEV_DEVICE *joy)
{
	if (joy->fd >= 0)
		close(joy->fd);

	joy->fd = -1;
	joy->buttons = 0;
}

static int fb_joydev_open_path(FB_JOYDEV_DEVICE *joy, const char *prefix, int index)
{
	char device_name[32];
	unsigned int version;

	if (snprintf(device_name, sizeof(device_name), "%s%d", prefix, index) >=
		(int)sizeof(device_name))
		return FALSE;

	joy->fd = open(device_name, O_RDONLY | O_NONBLOCK);
	if (joy->fd < 0)
		return FALSE;

	version = 0;
	if ((ioctl(joy->fd, FB_JOYDEV_GET_VERSION, &version) < 0) ||
		(version < 0x10000)) {
		fb_joydev_close(joy);
		return FALSE;
	}

	return TRUE;
}

static void fb_joydev_init(void)
{
	static const char *const device_path[] = {
		"/dev/input/js",
		"/dev/js",
		NULL
	};
	FB_JOYDEV_DEVICE *joy;
	int count;
	int i;
	int j;
	int axis;

	if (fb_joydev_inited)
		return;

	fb_hMemSet(fb_joydev, 0, sizeof(fb_joydev));
	for (i = 0; i < FB_JOYDEV_MAX_DEVICES; ++i)
		fb_joydev[i].fd = -1;

	count = 0;
	for (i = 0; (device_path[i] != NULL) && (count < FB_JOYDEV_MAX_DEVICES); ++i) {
		for (j = 0; (j < FB_JOYDEV_MAX_DEVICES) &&
					(count < FB_JOYDEV_MAX_DEVICES); ++j) {
			joy = &fb_joydev[count];
			if (!fb_joydev_open_path(joy, device_path[i], j))
				continue;

			for (axis = 0; axis < FB_JOYDEV_MAX_AXES; ++axis)
				joy->axis[axis] = -1000.0f;

			++count;
		}
	}

	fb_joydev_inited = TRUE;
}

static int fb_joydev_poll(FB_JOYDEV_DEVICE *joy)
{
	FB_JOYDEV_EVENT event;
	ssize_t bytes;

	while ((bytes = read(joy->fd, &event, sizeof(event))) == sizeof(event)) {
		switch (event.type & ~FB_JOYDEV_EVENT_INIT) {
		case FB_JOYDEV_EVENT_AXIS:
			if (event.number < FB_JOYDEV_MAX_AXES)
				joy->axis[event.number] = fb_joydev_normalize_axis(event.value);
			break;

		case FB_JOYDEV_EVENT_BUTTON:
			if (event.number < FB_JOYDEV_MAX_BUTTONS) {
				if (event.value)
					joy->buttons |= (1u << event.number);
				else
					joy->buttons &= ~(1u << event.number);
			}
			break;
		}
	}

	if ((bytes < 0) &&
		((errno == ENODEV) || (errno == ENXIO) || (errno == EIO))) {
		fb_joydev_close(joy);
		return FALSE;
	}

	return TRUE;
}

FBCALL int fb_GfxGetJoystick(int id,
							 ssize_t *buttons,
							 float *a1, float *a2,
							 float *a3, float *a4,
							 float *a5, float *a6,
							 float *a7, float *a8)
{
	FB_JOYDEV_DEVICE *joy;

	FB_GRAPHICS_LOCK( );

	fb_joydev_clear_outputs(buttons, a1, a2, a3, a4, a5, a6, a7, a8);

	if ((id < 0) || (id >= FB_JOYDEV_MAX_DEVICES)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	fb_joydev_init();

	joy = &fb_joydev[id];
	if (joy->fd < 0) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	if (!fb_joydev_poll(joy)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

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

	FB_GRAPHICS_UNLOCK( );
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of gfx_joydev_common.inc */
