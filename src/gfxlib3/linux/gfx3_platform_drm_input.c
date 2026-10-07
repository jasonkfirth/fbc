/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_platform_drm_input.c

    Purpose:

        Read Linux evdev devices and publish native keyboard and gamepad input.

    Responsibilities:

        - map evdev keycodes to FreeBASIC keyboard events
        - publish standard gamepad buttons, axes, and D-pad state
        - tolerate missing or inaccessible input devices

    This file intentionally does NOT contain:

        - DRM modesetting, GBM buffers, or EGL contexts
        - mouse or touch emulation
        - display lifecycle or renderer commands
*/

#include "gfx3_platform_drm_input.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#define FB_GFX3_DRM_INPUT_DEVICE_MAX 32u

typedef struct FB_GFX3_DRM_INPUT_DEVICE {
	int descriptor;
	int device_id;
	int keyboard;
	int gamepad;
	int shift_left;
	int shift_right;
	int caps_lock;
	unsigned char event_buffer[sizeof(struct input_event)];
	size_t event_buffer_length;
	ssize_t buttons;
	ssize_t dpad_keys;
	struct input_absinfo absolute[ABS_MAX + 1];
} FB_GFX3_DRM_INPUT_DEVICE;

struct FB_GFX3_DRM_INPUT_ADAPTER {
	FB_GFX3_INPUT_STATE *input;
	uint32_t device_count;
	FB_GFX3_DRM_INPUT_DEVICE device[FB_GFX3_DRM_INPUT_DEVICE_MAX];
};

/* ------------------------------------------------------------------------- */
/* Event code translation                                                     */
/* ------------------------------------------------------------------------- */

static int drm_input_test_bit(unsigned int bit, const unsigned long *bits,
	size_t bit_count)
{
	const size_t word_bits = sizeof(unsigned long) * CHAR_BIT;
	size_t word = (size_t)bit / word_bits;

	if ((bits == NULL) || (word >= bit_count))
		return FALSE;
	return (bits[word] & (1UL << (bit % word_bits))) != 0UL;
}

static int drm_input_key_scancode(unsigned int code)
{
	if ((code >= KEY_ESC) && (code <= KEY_F12))
		return (int)code;
	switch (code) {
	case KEY_RIGHTCTRL: return SC_CONTROL;
	case KEY_KPENTER: return SC_ENTER;
	case KEY_KPSLASH: return SC_SLASH;
	case KEY_RIGHTALT: return SC_ALTGR;
	case KEY_HOME: return SC_HOME;
	case KEY_UP: return SC_UP;
	case KEY_PAGEUP: return SC_PAGEUP;
	case KEY_LEFT: return SC_LEFT;
	case KEY_RIGHT: return SC_RIGHT;
	case KEY_END: return SC_END;
	case KEY_DOWN: return SC_DOWN;
	case KEY_PAGEDOWN: return SC_PAGEDOWN;
	case KEY_INSERT: return SC_INSERT;
	case KEY_DELETE: return SC_DELETE;
	case KEY_LEFTMETA: case KEY_RIGHTMETA: return SC_LWIN;
	default: return 0;
	}
}

static int drm_input_key_ascii(unsigned int code, int shifted, int caps_lock)
{
	static const char normal_digits[] = "1234567890";
	static const char shifted_digits[] = "!@#$%^&*()";
	int letter = 0;

	if ((code >= KEY_1) && (code <= KEY_0)) {
		unsigned int index = code - KEY_1;
		return shifted ? shifted_digits[index] : normal_digits[index];
	}
	switch (code) {
	case KEY_A: letter = 'a'; break; case KEY_B: letter = 'b'; break;
	case KEY_C: letter = 'c'; break; case KEY_D: letter = 'd'; break;
	case KEY_E: letter = 'e'; break; case KEY_F: letter = 'f'; break;
	case KEY_G: letter = 'g'; break; case KEY_H: letter = 'h'; break;
	case KEY_I: letter = 'i'; break; case KEY_J: letter = 'j'; break;
	case KEY_K: letter = 'k'; break; case KEY_L: letter = 'l'; break;
	case KEY_M: letter = 'm'; break; case KEY_N: letter = 'n'; break;
	case KEY_O: letter = 'o'; break; case KEY_P: letter = 'p'; break;
	case KEY_Q: letter = 'q'; break; case KEY_R: letter = 'r'; break;
	case KEY_S: letter = 's'; break; case KEY_T: letter = 't'; break;
	case KEY_U: letter = 'u'; break; case KEY_V: letter = 'v'; break;
	case KEY_W: letter = 'w'; break; case KEY_X: letter = 'x'; break;
	case KEY_Y: letter = 'y'; break; case KEY_Z: letter = 'z'; break;
	case KEY_SPACE: return ' ';
	case KEY_ENTER: case KEY_KPENTER: return '\r';
	case KEY_TAB: return '\t';
	case KEY_BACKSPACE: return '\b';
	case KEY_MINUS: return shifted ? '_' : '-';
	case KEY_EQUAL: return shifted ? '+' : '=';
	case KEY_LEFTBRACE: return shifted ? '{' : '[';
	case KEY_RIGHTBRACE: return shifted ? '}' : ']';
	case KEY_SEMICOLON: return shifted ? ':' : ';';
	case KEY_APOSTROPHE: return shifted ? '"' : '\'';
	case KEY_GRAVE: return shifted ? '~' : 96;
	case KEY_BACKSLASH: return shifted ? '|' : '\\';
	case KEY_COMMA: return shifted ? '<' : ',';
	case KEY_DOT: return shifted ? '>' : '.';
	case KEY_SLASH: return shifted ? '?' : '/';
	default: return 0;
	}
	if (letter != 0)
		return ((shifted != caps_lock) ? letter - ('a' - 'A') : letter);
	return 0;
}

static ssize_t drm_input_button(unsigned int code)
{
	switch (code) {
	case BTN_SOUTH: return XPAD_BUTTON_A;
	case BTN_EAST: return XPAD_BUTTON_B;
	case BTN_WEST: return XPAD_BUTTON_X;
	case BTN_NORTH: return XPAD_BUTTON_Y;
	case BTN_TL: return XPAD_BUTTON_L1;
	case BTN_TR: return XPAD_BUTTON_R1;
	case BTN_THUMBL: return XPAD_BUTTON_L3;
	case BTN_THUMBR: return XPAD_BUTTON_R3;
	case BTN_START: return XPAD_BUTTON_START;
	case BTN_SELECT: return XPAD_BUTTON_SELECT;
	/* Handheld device trees often use these codes for the missing pad keys. */
	case BTN_TRIGGER_HAPPY1: return XPAD_BUTTON_SELECT;
	case BTN_TRIGGER_HAPPY2: return XPAD_BUTTON_START;
	case BTN_TRIGGER_HAPPY3: return XPAD_BUTTON_L3;
	case BTN_TRIGGER_HAPPY4: return XPAD_BUTTON_R3;
	case BTN_MODE: return XPAD_BUTTON_GUIDE;
	case BTN_TL2: return XPAD_BUTTON_L2;
	case BTN_TR2: return XPAD_BUTTON_R2;
	default: return 0;
	}
}

static int drm_input_has_gamepad_buttons(const unsigned long *key_bits,
	size_t bit_count)
{
	static const unsigned int button_code[] = {
		BTN_SOUTH, BTN_EAST, BTN_WEST, BTN_NORTH,
		BTN_TL, BTN_TR, BTN_THUMBL, BTN_THUMBR,
		BTN_START, BTN_SELECT, BTN_MODE, BTN_TL2, BTN_TR2,
		BTN_TRIGGER_HAPPY1, BTN_TRIGGER_HAPPY2,
		BTN_TRIGGER_HAPPY3, BTN_TRIGGER_HAPPY4,
		BTN_DPAD_UP, BTN_DPAD_RIGHT, BTN_DPAD_DOWN, BTN_DPAD_LEFT
	};
	size_t index;

	/* Some handhelds publish their controls as EV_KEY without any EV_ABS axes. */
	for (index = 0u; index < sizeof(button_code) / sizeof(button_code[0]);
	    index++) {
		if (drm_input_test_bit(button_code[index], key_bits, bit_count))
			return TRUE;
	}
	return FALSE;
}

static ssize_t drm_input_dpad(unsigned int code)
{
	switch (code) {
	case BTN_DPAD_UP: case KEY_UP: return XPAD_DPAD_UP;
	case BTN_DPAD_RIGHT: case KEY_RIGHT: return XPAD_DPAD_RIGHT;
	case BTN_DPAD_DOWN: case KEY_DOWN: return XPAD_DPAD_DOWN;
	case BTN_DPAD_LEFT: case KEY_LEFT: return XPAD_DPAD_LEFT;
	default: return 0;
	}
}

/* ------------------------------------------------------------------------- */
/* Keyboard and controller snapshots                                         */
/* ------------------------------------------------------------------------- */

static float drm_input_axis(const struct input_absinfo *absolute)
{
	int64_t midpoint;
	double low_range;
	double high_range;
	double value;

	if ((absolute == NULL) || (absolute->maximum <= absolute->minimum))
		return 0.0f;
	midpoint = ((int64_t)absolute->minimum + absolute->maximum) / 2;
	low_range = (double)midpoint - (double)absolute->minimum;
	high_range = (double)absolute->maximum - (double)midpoint;
	if (((int64_t)absolute->value < midpoint) && (low_range > 0.0))
		value = ((double)absolute->value - (double)midpoint) / low_range;
	else if (high_range > 0.0)
		value = ((double)absolute->value - (double)midpoint) / high_range;
	else
		value = 0.0;
	if (value < -1.0)
		value = -1.0;
	if (value > 1.0)
		value = 1.0;
	return (float)value;
}

static float drm_input_trigger(const struct input_absinfo *absolute)
{
	double value;

	if ((absolute == NULL) || (absolute->maximum <= absolute->minimum))
		return 0.0f;
	value = ((double)absolute->value - absolute->minimum) /
		((double)absolute->maximum - absolute->minimum);
	if (value < 0.0)
		value = 0.0;
	if (value > 1.0)
		value = 1.0;
	return (float)value;
}

static void drm_input_publish_gamepad(FB_GFX3_DRM_INPUT_ADAPTER *adapter,
	FB_GFX3_DRM_INPUT_DEVICE *device)
{
	ssize_t dpad = device->dpad_keys;
	float axes[FB_GFX3_INPUT_GAMEPAD_AXIS_COUNT] = { 0.0f };

	axes[0] = drm_input_axis(&device->absolute[ABS_X]);
	axes[1] = drm_input_axis(&device->absolute[ABS_Y]);
	axes[2] = drm_input_axis(&device->absolute[ABS_RX]);
	axes[3] = drm_input_axis(&device->absolute[ABS_RY]);
	axes[4] = drm_input_trigger(&device->absolute[ABS_Z]);
	axes[5] = drm_input_trigger(&device->absolute[ABS_RZ]);
	if (device->absolute[ABS_HAT0X].value < 0)
		dpad |= XPAD_DPAD_LEFT;
	if (device->absolute[ABS_HAT0X].value > 0)
		dpad |= XPAD_DPAD_RIGHT;
	if (device->absolute[ABS_HAT0Y].value < 0)
		dpad |= XPAD_DPAD_UP;
	if (device->absolute[ABS_HAT0Y].value > 0)
		dpad |= XPAD_DPAD_DOWN;
	axes[6] = (float)(((dpad & XPAD_DPAD_RIGHT) != 0) -
		((dpad & XPAD_DPAD_LEFT) != 0));
	axes[7] = (float)(((dpad & XPAD_DPAD_DOWN) != 0) -
		((dpad & XPAD_DPAD_UP) != 0));
	(void)fb_gfx3_input_platform_gamepad_replace(adapter->input,
		device->device_id, TRUE, device->buttons, axes, axes[4], axes[5], dpad);
}

static void drm_input_keyboard_event(FB_GFX3_DRM_INPUT_ADAPTER *adapter,
	FB_GFX3_DRM_INPUT_DEVICE *device, const struct input_event *event)
{
	int scancode;
	int key;
	int type;
	int shifted;

	if (event->type != EV_KEY)
		return;
	if (event->code == KEY_LEFTSHIFT)
		device->shift_left = (event->value != 0);
	if (event->code == KEY_RIGHTSHIFT)
		device->shift_right = (event->value != 0);
	if ((event->code == KEY_CAPSLOCK) && (event->value == 1))
		device->caps_lock = !device->caps_lock;
	scancode = drm_input_key_scancode(event->code);
	if (scancode == 0)
		return;
	shifted = device->shift_left || device->shift_right;
	key = drm_input_key_ascii(event->code, shifted, device->caps_lock);
	if (key == 0)
		key = fb_hScancodeToExtendedKey(scancode);
	type = (event->value == 0) ? EVENT_KEY_RELEASE :
		((event->value == 2) ? EVENT_KEY_REPEAT : EVENT_KEY_PRESS);
	fb_gfx3_input_platform_key(adapter->input, type, scancode,
		((key > 0) && (key <= 0xFF)) ? key : 0);
	if ((event->value != 0) && (key > 0))
		fb_gfx3_input_platform_character(adapter->input, key, 1u);
}

static void drm_input_gamepad_event(FB_GFX3_DRM_INPUT_ADAPTER *adapter,
	FB_GFX3_DRM_INPUT_DEVICE *device, const struct input_event *event)
{
	ssize_t button;
	ssize_t dpad;

	if ((event->type == EV_ABS) && (event->code <= ABS_MAX)) {
		if (device->absolute[event->code].maximum <=
		    device->absolute[event->code].minimum)
			return;
		device->absolute[event->code].value = event->value;
	} else if ((event->type == EV_KEY) && (event->value != 2)) {
		button = drm_input_button(event->code);
		dpad = drm_input_dpad(event->code);
		if (event->value != 0) {
			device->buttons |= button;
			device->dpad_keys |= dpad;
		} else {
			device->buttons &= ~button;
			device->dpad_keys &= ~dpad;
		}
	} else {
		return;
	}
	drm_input_publish_gamepad(adapter, device);
}

static void drm_input_dispatch(FB_GFX3_DRM_INPUT_ADAPTER *adapter,
	FB_GFX3_DRM_INPUT_DEVICE *device, const struct input_event *event)
{
	if (device->keyboard)
		drm_input_keyboard_event(adapter, device, event);
	if (device->gamepad)
		drm_input_gamepad_event(adapter, device, event);
}

/* ------------------------------------------------------------------------- */
/* Device discovery and lifecycle                                             */
/* ------------------------------------------------------------------------- */

static void drm_input_close_device(FB_GFX3_DRM_INPUT_ADAPTER *adapter,
	FB_GFX3_DRM_INPUT_DEVICE *device)
{
	if (device->descriptor < 0)
		return;
	if (device->gamepad)
		(void)fb_gfx3_input_platform_gamepad_replace(adapter->input,
			device->device_id, FALSE, 0, NULL, 0.0f, 0.0f, 0);
	close(device->descriptor);
	device->descriptor = -1;
}

static void drm_input_open_devices(FB_GFX3_DRM_INPUT_ADAPTER *adapter)
{
	DIR *directory = opendir("/dev/input");
	struct dirent *entry;

	if (directory == NULL)
		return;
	while ((entry = readdir(directory)) != NULL) {
		unsigned long event_bits[(EV_MAX / (sizeof(unsigned long) * CHAR_BIT)) + 1u];
		unsigned long key_bits[(KEY_MAX / (sizeof(unsigned long) * CHAR_BIT)) + 1u];
		unsigned long key_state[(KEY_MAX / (sizeof(unsigned long) * CHAR_BIT)) + 1u];
		unsigned long absolute_bits[(ABS_MAX / (sizeof(unsigned long) * CHAR_BIT)) + 1u];
		char path[PATH_MAX];
		struct stat status;
		FB_GFX3_DRM_INPUT_DEVICE *device;
		int descriptor;
		int keyboard;
		int gamepad;
		unsigned int code;
		unsigned int device_minor;
		size_t word;

		if (strncmp(entry->d_name, "event", 5u) != 0)
			continue;
		if (adapter->device_count >= FB_GFX3_DRM_INPUT_DEVICE_MAX)
			break;
		if (snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name) < 0)
			continue;
		descriptor = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
		if (descriptor < 0)
			continue;
		memset(event_bits, 0, sizeof(event_bits));
		memset(key_bits, 0, sizeof(key_bits));
		memset(key_state, 0, sizeof(key_state));
		memset(absolute_bits, 0, sizeof(absolute_bits));
		if ((ioctl(descriptor, EVIOCGBIT(0, sizeof(event_bits)), event_bits) < 0) ||
		    (ioctl(descriptor, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0)) {
			close(descriptor);
			continue;
		}
		if (drm_input_test_bit(EV_ABS, event_bits,
		    sizeof(event_bits) / sizeof(event_bits[0])))
			(void)ioctl(descriptor,
				EVIOCGBIT(EV_ABS, sizeof(absolute_bits)), absolute_bits);
		keyboard = drm_input_test_bit(KEY_A, key_bits,
			sizeof(key_bits) / sizeof(key_bits[0])) &&
			drm_input_test_bit(KEY_Z, key_bits,
				sizeof(key_bits) / sizeof(key_bits[0])) &&
			drm_input_test_bit(KEY_ENTER, key_bits,
				sizeof(key_bits) / sizeof(key_bits[0]));
		gamepad = drm_input_test_bit(EV_KEY, event_bits,
			sizeof(event_bits) / sizeof(event_bits[0])) &&
			(drm_input_has_gamepad_buttons(key_bits,
				sizeof(key_bits) / sizeof(key_bits[0])) ||
			 (drm_input_test_bit(EV_ABS, event_bits,
				sizeof(event_bits) / sizeof(event_bits[0])) &&
			  (drm_input_test_bit(ABS_X, absolute_bits,
				sizeof(absolute_bits) / sizeof(absolute_bits[0])) ||
			   drm_input_test_bit(ABS_HAT0X, absolute_bits,
				sizeof(absolute_bits) / sizeof(absolute_bits[0])))));
		if (!keyboard && !gamepad) {
			close(descriptor);
			continue;
		}
		if ((fstat(descriptor, &status) != 0) ||
		    ((device_minor = minor(status.st_rdev)) > (unsigned int)INT_MAX)) {
			close(descriptor);
			continue;
		}
		device = &adapter->device[adapter->device_count];
		memset(device, 0, sizeof(*device));
		device->descriptor = descriptor;
		device->device_id = (int)device_minor;
		device->keyboard = keyboard;
		device->gamepad = gamepad;
		if (device->gamepad) {
			for (word = 0; word < sizeof(absolute_bits) /
			    sizeof(absolute_bits[0]); word++) {
				unsigned int first = (unsigned int)(word *
					sizeof(unsigned long) * CHAR_BIT);
				unsigned int offset;

				for (offset = 0u; (offset < sizeof(unsigned long) * CHAR_BIT) &&
				    (first + offset <= ABS_MAX); offset++) {
					unsigned int axis = first + offset;
					if (drm_input_test_bit(axis, absolute_bits,
					    sizeof(absolute_bits) /
					    sizeof(absolute_bits[0])))
						(void)ioctl(descriptor, EVIOCGABS(axis),
							&device->absolute[axis]);
				}
			}
			/*
				EVIOCGBIT above describes supported key codes. EVIOCGKEY
				returns keys currently held. Mixing those bitmaps would make
				every advertised D-pad direction appear pressed at startup.
			*/
			if (ioctl(descriptor, EVIOCGKEY(sizeof(key_state)), key_state) >= 0) {
				for (code = 0u; code <= (unsigned int)KEY_MAX; code++) {
					if (!drm_input_test_bit(code, key_state,
					    sizeof(key_state) / sizeof(key_state[0])))
						continue;
					device->buttons |= drm_input_button(code);
					device->dpad_keys |= drm_input_dpad(code);
				}
			}
			drm_input_publish_gamepad(adapter, device);
		}
		adapter->device_count++;
	}
	closedir(directory);
}

FB_GFX3_DRM_INPUT_ADAPTER *fb_gfx3_platform_drm_input_create(
	FB_GFX3_INPUT_STATE *input)
{
	FB_GFX3_DRM_INPUT_ADAPTER *adapter;
	uint32_t index;

	if (input == NULL)
		return NULL;
	adapter = (FB_GFX3_DRM_INPUT_ADAPTER *)calloc(1, sizeof(*adapter));
	if (adapter == NULL)
		return NULL;
	adapter->input = input;
	for (index = 0u; index < FB_GFX3_DRM_INPUT_DEVICE_MAX; index++)
		adapter->device[index].descriptor = -1;
	drm_input_open_devices(adapter);
	return adapter;
}

void fb_gfx3_platform_drm_input_pump(FB_GFX3_DRM_INPUT_ADAPTER *adapter)
{
	uint32_t index;

	if (adapter == NULL)
		return;
	for (index = 0u; index < adapter->device_count; index++) {
		FB_GFX3_DRM_INPUT_DEVICE *device = &adapter->device[index];

		while (device->descriptor >= 0) {
			ssize_t amount = read(device->descriptor,
				device->event_buffer + device->event_buffer_length,
				sizeof(device->event_buffer) - device->event_buffer_length);
			if (amount > 0) {
				device->event_buffer_length += (size_t)amount;
				if (device->event_buffer_length == sizeof(device->event_buffer)) {
					struct input_event event;
					memcpy(&event, device->event_buffer, sizeof(event));
					device->event_buffer_length = 0u;
					drm_input_dispatch(adapter, device, &event);
				}
				continue;
			}
			if ((amount < 0) && (errno == EINTR))
				continue;
			if ((amount < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
				break;
			drm_input_close_device(adapter, device);
			break;
		}
	}
}

void fb_gfx3_platform_drm_input_destroy(
	FB_GFX3_DRM_INPUT_ADAPTER *adapter)
{
	uint32_t index;

	if (adapter == NULL)
		return;
	for (index = 0u; index < adapter->device_count; index++)
		drm_input_close_device(adapter, &adapter->device[index]);
	free(adapter);
}

/* end of linux/gfx3_platform_drm_input.c */
