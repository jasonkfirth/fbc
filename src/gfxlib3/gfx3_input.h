/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_input.h

    Purpose:

        Define the synchronized input state shared by a native window thread
        and FreeBASIC's graphics compatibility entry points.

    Responsibilities:

        - retain keyboard, mouse, and focus state outside backend objects
        - own bounded SCREENEVENT and INKEY queues
        - accept platform events without taking FreeBASIC runtime locks
        - carry asynchronous SETMOUSE requests to the native window owner

    This file intentionally does NOT contain:

        - Win32, X11, Wayland, or other native event declarations
        - renderer commands or GPU resources
        - graphical console drawing
*/

#ifndef __FB_GFX3_INPUT_H__
#define __FB_GFX3_INPUT_H__

#include "fb_gfx3.h"

#define FB_GFX3_INPUT_KEY_COUNT 128u
#define FB_GFX3_INPUT_KEY_BUFFER_LENGTH 16u
#define FB_GFX3_INPUT_TOUCH_MAX 16u
#define FB_GFX3_INPUT_GAMEPAD_MAX 16u
#define FB_GFX3_INPUT_GAMEPAD_AXIS_COUNT 8u

enum FB_GFX3_MOUSE_REQUEST_FLAGS {
	FB_GFX3_MOUSE_REQUEST_POSITION = 0x01u,
	FB_GFX3_MOUSE_REQUEST_CURSOR = 0x02u,
	FB_GFX3_MOUSE_REQUEST_CLIP = 0x04u
};

enum FB_GFX3_WINDOW_REQUEST_FLAGS {
	FB_GFX3_WINDOW_REQUEST_POSITION = 0x01u
};

typedef struct FB_GFX3_MOUSE_REQUEST {
	uint32_t flags;
	int x;
	int y;
	int cursor;
	int clip;
} FB_GFX3_MOUSE_REQUEST;

typedef struct FB_GFX3_WINDOW_REQUEST {
	uint32_t flags;
	int x;
	int y;
} FB_GFX3_WINDOW_REQUEST;

/*
	Contacts are deliberately stored in the common input state instead of an
	Android-only side table.  Public GETTOUCH calls run on the BASIC thread,
	while NativeActivity delivers motion events on its looper thread.  Keeping
	the snapshot beside mouse state gives both paths one synchronization rule
	and leaves room for another native-touch platform without changing the ABI.
*/
typedef struct FB_GFX3_TOUCH_CONTACT {
	int id;
	int x;
	int y;
} FB_GFX3_TOUCH_CONTACT;

/*
	Gamepads are stored by the platform device identifier but exposed by their
	stable first-seen slot, matching gfxlib2's GETJOYSTICK and GETXPAD contract.
	A seen-but-disconnected slot is retained so GETXPAD can distinguish a device
	that was unplugged from one that was never present.
	The compact snapshot is intentionally independent of Android so future
	platform adapters can publish the same public data without a second query
	path or a renderer-thread dependency.
*/
typedef struct FB_GFX3_GAMEPAD_STATE {
	int device_id;
	int seen;
	int connected;
	ssize_t buttons;
	ssize_t dpad;
	float axis[FB_GFX3_INPUT_GAMEPAD_AXIS_COUNT];
	float left_trigger;
	float right_trigger;
} FB_GFX3_GAMEPAD_STATE;

typedef struct FB_GFX3_INPUT_STATE {
	FBMUTEX *mutex;
	EVENT event_queue[MAX_EVENTS];
	unsigned char key[FB_GFX3_INPUT_KEY_COUNT];
	int key_buffer[FB_GFX3_INPUT_KEY_BUFFER_LENGTH];
	FB_GFX3_MOUSE_REQUEST mouse_request;
	FB_GFX3_WINDOW_REQUEST window_request;
	uintptr_t native_window;
	uintptr_t native_display;
	uint32_t width;
	uint32_t height;
	uint32_t pending_width;
	uint32_t pending_height;
	uint32_t event_head;
	uint32_t event_tail;
	uint32_t key_head;
	uint32_t key_tail;
	int mouse_x;
	int mouse_y;
	int mouse_z;
	int mouse_w;
	int mouse_buttons;
	int mouse_clip;
	int cursor_visible;
	int window_x;
	int window_y;
	int desktop_width;
	int desktop_height;
	FB_GFX3_TOUCH_CONTACT touch[FB_GFX3_INPUT_TOUCH_MAX];
	FB_GFX3_GAMEPAD_STATE gamepad[FB_GFX3_INPUT_GAMEPAD_MAX];
	uint32_t touch_count;
	int mouse_inside;
	int focused;
	int native_touch;
	int resizable;
	int initialized;
} FB_GFX3_INPUT_STATE;

int fb_gfx3_input_init(FB_GFX3_INPUT_STATE *input, uint32_t width,
	uint32_t height);
void fb_gfx3_input_destroy(FB_GFX3_INPUT_STATE *input);

void fb_gfx3_input_platform_focus(FB_GFX3_INPUT_STATE *input, int focused);
void fb_gfx3_input_platform_mouse_enter(FB_GFX3_INPUT_STATE *input);
void fb_gfx3_input_platform_mouse_exit(FB_GFX3_INPUT_STATE *input);
void fb_gfx3_input_platform_mouse_move(FB_GFX3_INPUT_STATE *input,
	int x, int y);
void fb_gfx3_input_platform_mouse_button(FB_GFX3_INPUT_STATE *input,
	int button, int pressed, int double_click);
void fb_gfx3_input_platform_mouse_wheel(FB_GFX3_INPUT_STATE *input,
	int horizontal, int direction);
int fb_gfx3_input_platform_touch_replace(FB_GFX3_INPUT_STATE *input,
	const FB_GFX3_TOUCH_CONTACT *contacts, uint32_t count);
int fb_gfx3_input_touch_snapshot(FB_GFX3_INPUT_STATE *input, ssize_t index,
	ssize_t *count, int *x, int *y, int *id);
int fb_gfx3_input_platform_gamepad_motion(FB_GFX3_INPUT_STATE *input,
	int device_id, const float *axis, float left_trigger, float right_trigger,
	ssize_t dpad);
int fb_gfx3_input_platform_gamepad_replace(FB_GFX3_INPUT_STATE *input,
	int device_id, int connected, ssize_t buttons, const float *axis,
	float left_trigger, float right_trigger, ssize_t dpad);
int fb_gfx3_input_platform_gamepad_key(FB_GFX3_INPUT_STATE *input,
	int device_id, ssize_t button, ssize_t dpad, int pressed);
int fb_gfx3_input_gamepad_snapshot(FB_GFX3_INPUT_STATE *input, ssize_t index,
	FB_GFX3_GAMEPAD_STATE *gamepad);
void fb_gfx3_input_platform_key(FB_GFX3_INPUT_STATE *input, int type,
	int scancode, int ascii);
void fb_gfx3_input_platform_character(FB_GFX3_INPUT_STATE *input, int key,
	uint32_t repeat_count);
void fb_gfx3_input_platform_close(FB_GFX3_INPUT_STATE *input);
void fb_gfx3_input_set_resizable(FB_GFX3_INPUT_STATE *input, int enabled);
void fb_gfx3_input_platform_resize(FB_GFX3_INPUT_STATE *input,
	uint32_t width, uint32_t height);
int fb_gfx3_input_take_resize(FB_GFX3_INPUT_STATE *input,
	uint32_t *width, uint32_t *height);
void fb_gfx3_input_complete_resize(FB_GFX3_INPUT_STATE *input,
	uint32_t width, uint32_t height);
int fb_gfx3_input_platform_take_mouse_request(FB_GFX3_INPUT_STATE *input,
	FB_GFX3_MOUSE_REQUEST *request);
void fb_gfx3_input_platform_window_info(FB_GFX3_INPUT_STATE *input,
	uintptr_t native_window, uintptr_t native_display, int x, int y,
	int desktop_width, int desktop_height);
void fb_gfx3_input_platform_window_moved(FB_GFX3_INPUT_STATE *input,
	int x, int y);
int fb_gfx3_input_platform_take_window_request(FB_GFX3_INPUT_STATE *input,
	FB_GFX3_WINDOW_REQUEST *request);
int fb_gfx3_input_window_snapshot(FB_GFX3_INPUT_STATE *input,
	uintptr_t *native_window, uintptr_t *native_display, int *x, int *y,
	int *desktop_width, int *desktop_height);
int fb_gfx3_input_request_window_position(FB_GFX3_INPUT_STATE *input,
	int x, int y);

void fb_gfx3_input_install_hooks_locked(void);

#endif

/* end of gfx3_input.h */
