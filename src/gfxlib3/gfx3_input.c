/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_input.c

    Purpose:

        Implement platform-neutral synchronized keyboard, mouse, and event
        state for an active gfxlib3 mode.

    Responsibilities:

        - serialize native event production with BASIC input queries
		- preserve gfxlib2 queue overflow and event-peek behavior
		- retain scan-code, mouse, focus, and cursor-request state

    This file intentionally does NOT contain:

        - native virtual-key or mouse-message translation
		- window creation, cursor clipping calls, or operating-system handles
		- FreeBASIC public input hooks or graphical text-console rendering
*/

#include "gfx3_input.h"

_Static_assert((MAX_EVENTS & (MAX_EVENTS - 1)) == 0,
	"MAX_EVENTS must remain a power of two");
_Static_assert((FB_GFX3_INPUT_KEY_BUFFER_LENGTH &
	(FB_GFX3_INPUT_KEY_BUFFER_LENGTH - 1u)) == 0,
	"the INKEY queue length must remain a power of two");

/* ------------------------------------------------------------------------- */
/* Bounded queues and platform event producers                               */
/* ------------------------------------------------------------------------- */

static void input_post_event_locked(FB_GFX3_INPUT_STATE *input,
	const EVENT *event)
{
	uint32_t next_tail;

	input->event_queue[input->event_tail] = *event;
	next_tail = (input->event_tail + 1u) & (MAX_EVENTS - 1u);
	if (next_tail == input->event_head)
		input->event_head = (input->event_head + 1u) &
			(MAX_EVENTS - 1u);
	input->event_tail = next_tail;
}

static void input_post_key_locked(FB_GFX3_INPUT_STATE *input, int key)
{
	uint32_t next_tail;

	input->key_buffer[input->key_tail] = key;
	next_tail = (input->key_tail + 1u) &
		(FB_GFX3_INPUT_KEY_BUFFER_LENGTH - 1u);
	if (next_tail == input->key_head)
		input->key_head = (input->key_head + 1u) &
			(FB_GFX3_INPUT_KEY_BUFFER_LENGTH - 1u);
	input->key_tail = next_tail;
}

int fb_gfx3_input_init(FB_GFX3_INPUT_STATE *input, uint32_t width,
	uint32_t height)
{
	if ((input == NULL) || (width == 0) || (height == 0) ||
	    (width > INT_MAX) || (height > INT_MAX))
		return FB_GFX3_INVALID;
	memset(input, 0, sizeof(*input));
	input->mutex = fb_MutexCreate();
	if (input->mutex == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	input->width = width;
	input->height = height;
	input->mouse_x = -1;
	input->mouse_y = -1;
	input->cursor_visible = TRUE;
	input->initialized = TRUE;
	return FB_GFX3_OK;
}

void fb_gfx3_input_destroy(FB_GFX3_INPUT_STATE *input)
{
	FBMUTEX *mutex;

	if ((input == NULL) || (input->mutex == NULL))
		return;
	mutex = input->mutex;
	fb_MutexLock(mutex);
	input->initialized = FALSE;
	input->mutex = NULL;
	fb_MutexUnlock(mutex);
	fb_MutexDestroy(mutex);
	memset(input, 0, sizeof(*input));
}

void fb_gfx3_input_platform_focus(FB_GFX3_INPUT_STATE *input, int focused)
{
	EVENT event;

	if ((input == NULL) || (input->mutex == NULL))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized && (input->focused != (focused != FALSE))) {
		input->focused = (focused != FALSE);
		memset(input->key, 0, sizeof(input->key));
		input->mouse_buttons = 0;
		if (!input->focused) {
			input->touch_count = 0;
			memset(input->touch, 0, sizeof(input->touch));
		}
		if (!input->focused && input->mouse_inside) {
			input->mouse_inside = FALSE;
			memset(&event, 0, sizeof(event));
			event.type = EVENT_MOUSE_EXIT;
			input_post_event_locked(input, &event);
		}
		memset(&event, 0, sizeof(event));
		event.type = input->focused ? EVENT_WINDOW_GOT_FOCUS :
			EVENT_WINDOW_LOST_FOCUS;
		input_post_event_locked(input, &event);
	}
	fb_MutexUnlock(input->mutex);
}

void fb_gfx3_input_platform_mouse_enter(FB_GFX3_INPUT_STATE *input)
{
	EVENT event;

	if ((input == NULL) || (input->mutex == NULL))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized && !input->mouse_inside) {
		input->mouse_inside = TRUE;
		memset(&event, 0, sizeof(event));
		event.type = EVENT_MOUSE_ENTER;
		input_post_event_locked(input, &event);
	}
	fb_MutexUnlock(input->mutex);
}

void fb_gfx3_input_platform_mouse_exit(FB_GFX3_INPUT_STATE *input)
{
	EVENT event;

	if ((input == NULL) || (input->mutex == NULL))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized && input->mouse_inside) {
		input->mouse_inside = FALSE;
		memset(&event, 0, sizeof(event));
		event.type = EVENT_MOUSE_EXIT;
		input_post_event_locked(input, &event);
	}
	fb_MutexUnlock(input->mutex);
}

void fb_gfx3_input_platform_mouse_move(FB_GFX3_INPUT_STATE *input,
	int x, int y)
{
	EVENT event;
	int old_x;
	int old_y;

	if ((input == NULL) || (input->mutex == NULL))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		old_x = input->mouse_x;
		old_y = input->mouse_y;
		input->mouse_x = x;
		input->mouse_y = y;
		input->mouse_inside = TRUE;
		memset(&event, 0, sizeof(event));
		event.type = EVENT_MOUSE_MOVE;
		event.x = x;
		event.y = y;
		if ((old_x >= 0) && (old_y >= 0)) {
			event.dx = x - old_x;
			event.dy = y - old_y;
		}
		if ((event.dx != 0) || (event.dy != 0))
			input_post_event_locked(input, &event);
	}
	fb_MutexUnlock(input->mutex);
}

void fb_gfx3_input_platform_mouse_button(FB_GFX3_INPUT_STATE *input,
	int button, int pressed, int double_click)
{
	EVENT event;

	if ((input == NULL) || (input->mutex == NULL) ||
	    ((button & (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_MIDDLE |
	      BUTTON_X1 | BUTTON_X2)) == 0))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		if (pressed)
			input->mouse_buttons |= button;
		else
			input->mouse_buttons &= ~button;
		memset(&event, 0, sizeof(event));
		event.type = double_click ? EVENT_MOUSE_DOUBLE_CLICK :
			(pressed ? EVENT_MOUSE_BUTTON_PRESS :
			EVENT_MOUSE_BUTTON_RELEASE);
		event.button = button;
		input_post_event_locked(input, &event);
	}
	fb_MutexUnlock(input->mutex);
}

/* ------------------------------------------------------------------------- */
/* Native touch snapshots                                                     */
/* ------------------------------------------------------------------------- */

int fb_gfx3_input_platform_touch_replace(FB_GFX3_INPUT_STATE *input,
	const FB_GFX3_TOUCH_CONTACT *contacts, uint32_t count)
{
	int previous_count;

	if ((input == NULL) || (input->mutex == NULL) ||
	    ((count > 0) && (contacts == NULL)) ||
	    (count > FB_GFX3_INPUT_TOUCH_MAX))
		return -1;
	fb_MutexLock(input->mutex);
	if (!input->initialized) {
		fb_MutexUnlock(input->mutex);
		return -1;
	}
	previous_count = (int)input->touch_count;
	input->native_touch = TRUE;
	input->touch_count = count;
	memset(input->touch, 0, sizeof(input->touch));
	if (count > 0)
		memcpy(input->touch, contacts, count * sizeof(*contacts));
	fb_MutexUnlock(input->mutex);
	return previous_count;
}

int fb_gfx3_input_touch_snapshot(FB_GFX3_INPUT_STATE *input, ssize_t index,
	ssize_t *count, int *x, int *y, int *id)
{
	uint32_t contact_count;
	int supported;

	if ((input == NULL) || (input->mutex == NULL))
		return FALSE;
	fb_MutexLock(input->mutex);
	supported = input->initialized && input->native_touch;
	contact_count = input->touch_count;
	if (count != NULL)
		*count = supported ? (ssize_t)contact_count : 0;
	if (supported && (index >= 0) && ((uint32_t)index < contact_count)) {
		if (x != NULL)
			*x = input->touch[index].x;
		if (y != NULL)
			*y = input->touch[index].y;
		if (id != NULL)
			*id = input->touch[index].id;
		fb_MutexUnlock(input->mutex);
		return TRUE;
	}
	fb_MutexUnlock(input->mutex);
	return supported ? TRUE : FALSE;
}

/* ------------------------------------------------------------------------- */
/* Gamepad snapshots                                                          */
/* ------------------------------------------------------------------------- */

static FB_GFX3_GAMEPAD_STATE *input_gamepad_slot_locked(
	FB_GFX3_INPUT_STATE *input, int device_id, int create)
{
	FB_GFX3_GAMEPAD_STATE *free_slot = NULL;
	uint32_t i;

	if (device_id < 0)
		return NULL;
	for (i = 0; i < FB_GFX3_INPUT_GAMEPAD_MAX; ++i) {
		if (input->gamepad[i].seen &&
		    (input->gamepad[i].device_id == device_id))
			return &input->gamepad[i];
		if (!input->gamepad[i].seen && (free_slot == NULL))
			free_slot = &input->gamepad[i];
	}
	if (!create || (free_slot == NULL))
		return NULL;
	memset(free_slot, 0, sizeof(*free_slot));
	free_slot->device_id = device_id;
	free_slot->seen = TRUE;
	return free_slot;
}

int fb_gfx3_input_platform_gamepad_motion(FB_GFX3_INPUT_STATE *input,
	int device_id, const float *axis, float left_trigger, float right_trigger,
	ssize_t dpad)
{
	FB_GFX3_GAMEPAD_STATE *gamepad;

	if ((input == NULL) || (input->mutex == NULL) || (axis == NULL))
		return FB_GFX3_INVALID;
	fb_MutexLock(input->mutex);
	if (!input->initialized) {
		fb_MutexUnlock(input->mutex);
		return FB_GFX3_CLOSED;
	}
	gamepad = input_gamepad_slot_locked(input, device_id, TRUE);
	if (gamepad == NULL) {
		fb_MutexUnlock(input->mutex);
		return FB_GFX3_EXHAUSTED;
	}
	memcpy(gamepad->axis, axis, sizeof(gamepad->axis));
	gamepad->connected = TRUE;
	gamepad->left_trigger = left_trigger;
	gamepad->right_trigger = right_trigger;
	gamepad->dpad = dpad;
	fb_MutexUnlock(input->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_input_platform_gamepad_replace(FB_GFX3_INPUT_STATE *input,
	int device_id, int connected, ssize_t buttons, const float *axis,
	float left_trigger, float right_trigger, ssize_t dpad)
{
	FB_GFX3_GAMEPAD_STATE *gamepad;

	if ((input == NULL) || (input->mutex == NULL) ||
	    (connected && (axis == NULL)))
		return FB_GFX3_INVALID;
	fb_MutexLock(input->mutex);
	if (!input->initialized) {
		fb_MutexUnlock(input->mutex);
		return FB_GFX3_CLOSED;
	}
	gamepad = input_gamepad_slot_locked(input, device_id, connected);
	if (gamepad == NULL) {
		fb_MutexUnlock(input->mutex);
		return connected ? FB_GFX3_EXHAUSTED : FB_GFX3_OK;
	}
	gamepad->connected = (connected != FALSE);
	if (gamepad->connected) {
		gamepad->buttons = buttons;
		memcpy(gamepad->axis, axis, sizeof(gamepad->axis));
		gamepad->left_trigger = left_trigger;
		gamepad->right_trigger = right_trigger;
		gamepad->dpad = dpad;
	}
	fb_MutexUnlock(input->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_input_platform_gamepad_key(FB_GFX3_INPUT_STATE *input,
	int device_id, ssize_t button, ssize_t dpad, int pressed)
{
	FB_GFX3_GAMEPAD_STATE *gamepad;

	if ((input == NULL) || (input->mutex == NULL) ||
	    ((button == 0) && (dpad == 0)))
		return FB_GFX3_INVALID;
	fb_MutexLock(input->mutex);
	if (!input->initialized) {
		fb_MutexUnlock(input->mutex);
		return FB_GFX3_CLOSED;
	}
	gamepad = input_gamepad_slot_locked(input, device_id, TRUE);
	if (gamepad == NULL) {
		fb_MutexUnlock(input->mutex);
		return FB_GFX3_EXHAUSTED;
	}
	gamepad->connected = TRUE;
	if (pressed) {
		gamepad->buttons |= button;
		gamepad->dpad |= dpad;
	} else {
		gamepad->buttons &= ~button;
		gamepad->dpad &= ~dpad;
	}
	fb_MutexUnlock(input->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_input_gamepad_snapshot(FB_GFX3_INPUT_STATE *input,
	ssize_t index, FB_GFX3_GAMEPAD_STATE *gamepad)
{
	int available = FALSE;

	if ((input == NULL) || (input->mutex == NULL) || (gamepad == NULL) ||
	    (index < 0) || ((uint32_t)index >= FB_GFX3_INPUT_GAMEPAD_MAX))
		return FALSE;
	fb_MutexLock(input->mutex);
	if (input->initialized && input->gamepad[index].seen) {
		*gamepad = input->gamepad[index];
		available = TRUE;
	}
	fb_MutexUnlock(input->mutex);
	return available;
}

void fb_gfx3_input_platform_mouse_wheel(FB_GFX3_INPUT_STATE *input,
	int horizontal, int direction)
{
	EVENT event;

	if ((input == NULL) || (input->mutex == NULL) || (direction == 0))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		memset(&event, 0, sizeof(event));
		if (horizontal) {
			input->mouse_w += (direction > 0) ? 1 : -1;
			event.type = EVENT_MOUSE_HWHEEL;
			event.w = input->mouse_w;
		} else {
			input->mouse_z += (direction > 0) ? 1 : -1;
			event.type = EVENT_MOUSE_WHEEL;
			event.z = input->mouse_z;
		}
		input_post_event_locked(input, &event);
	}
	fb_MutexUnlock(input->mutex);
}

void fb_gfx3_input_platform_key(FB_GFX3_INPUT_STATE *input, int type,
	int scancode, int ascii)
{
	EVENT event;

	if ((input == NULL) || (input->mutex == NULL) ||
	    ((type != EVENT_KEY_PRESS) && (type != EVENT_KEY_RELEASE) &&
	     (type != EVENT_KEY_REPEAT)))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		if ((scancode >= 0) &&
		    ((uint32_t)scancode < FB_GFX3_INPUT_KEY_COUNT))
			input->key[scancode] = (type != EVENT_KEY_RELEASE);
		memset(&event, 0, sizeof(event));
		event.type = type;
		event.scancode = scancode;
		event.ascii = ((ascii >= 0) && (ascii <= 0xFF)) ? ascii : 0;
		input_post_event_locked(input, &event);
	}
	fb_MutexUnlock(input->mutex);
}

void fb_gfx3_input_platform_character(FB_GFX3_INPUT_STATE *input, int key,
	uint32_t repeat_count)
{
	if ((input == NULL) || (input->mutex == NULL) || (key <= 0))
		return;
	if (repeat_count > FB_GFX3_INPUT_KEY_BUFFER_LENGTH)
		repeat_count = FB_GFX3_INPUT_KEY_BUFFER_LENGTH;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		while (repeat_count > 0) {
			input_post_key_locked(input, key);
			repeat_count--;
		}
	}
	fb_MutexUnlock(input->mutex);
}

void fb_gfx3_input_platform_close(FB_GFX3_INPUT_STATE *input)
{
	EVENT event;

	if ((input == NULL) || (input->mutex == NULL))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		input_post_key_locked(input, KEY_QUIT);
		memset(&event, 0, sizeof(event));
		event.type = EVENT_WINDOW_CLOSE;
		input_post_event_locked(input, &event);
	}
	fb_MutexUnlock(input->mutex);
}

/* ------------------------------------------------------------------------- */
/* Coalesced native-window resize requests                                   */
/* ------------------------------------------------------------------------- */

void fb_gfx3_input_set_resizable(FB_GFX3_INPUT_STATE *input, int enabled)
{
	if ((input == NULL) || (input->mutex == NULL))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		input->resizable = (enabled != FALSE);
		if (!input->resizable) {
			input->pending_width = 0u;
			input->pending_height = 0u;
		}
	}
	fb_MutexUnlock(input->mutex);
}

/*
	The platform pump can run on the renderer thread, while BASIC owns the
	graphics lock on another thread.  Publishing dimensions through the input
	mutex avoids reversing those locks and naturally coalesces a resize drag.
*/
void fb_gfx3_input_platform_resize(FB_GFX3_INPUT_STATE *input,
	uint32_t width, uint32_t height)
{
	if ((input == NULL) || (input->mutex == NULL) || (width == 0u) ||
	    (height == 0u) || (width > INT_MAX) || (height > INT_MAX))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized && input->resizable) {
		input->pending_width = width;
		input->pending_height = height;
	}
	fb_MutexUnlock(input->mutex);
}

int fb_gfx3_input_take_resize(FB_GFX3_INPUT_STATE *input,
	uint32_t *width, uint32_t *height)
{
	int available = FALSE;

	if ((input == NULL) || (input->mutex == NULL) || (width == NULL) ||
	    (height == NULL))
		return FALSE;
	fb_MutexLock(input->mutex);
	if (input->initialized && input->resizable &&
	    (input->pending_width != 0u) && (input->pending_height != 0u)) {
		*width = input->pending_width;
		*height = input->pending_height;
		available = TRUE;
	}
	fb_MutexUnlock(input->mutex);
	return available;
}

void fb_gfx3_input_complete_resize(FB_GFX3_INPUT_STATE *input,
	uint32_t width, uint32_t height)
{
	EVENT event;
	int changed;

	if ((input == NULL) || (input->mutex == NULL) || (width == 0u) ||
	    (height == 0u))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized && input->resizable) {
		changed = (input->width != width) || (input->height != height);
		input->width = width;
		input->height = height;
		if ((input->pending_width == width) &&
		    (input->pending_height == height)) {
			input->pending_width = 0u;
			input->pending_height = 0u;
		}
		if (changed) {
			memset(&event, 0, sizeof(event));
			event.type = EVENT_WINDOW_RESIZE;
			event.width = (int)width;
			event.height = (int)height;
			input_post_event_locked(input, &event);
		}
	}
	fb_MutexUnlock(input->mutex);
}

int fb_gfx3_input_platform_take_mouse_request(FB_GFX3_INPUT_STATE *input,
	FB_GFX3_MOUSE_REQUEST *request)
{
	int available = FALSE;

	if ((input == NULL) || (request == NULL) || (input->mutex == NULL))
		return FALSE;
	fb_MutexLock(input->mutex);
	if (input->initialized && (input->mouse_request.flags != 0)) {
		*request = input->mouse_request;
		input->mouse_request.flags = 0;
		available = TRUE;
	}
	fb_MutexUnlock(input->mutex);
	return available;
}

void fb_gfx3_input_platform_window_info(FB_GFX3_INPUT_STATE *input,
	uintptr_t native_window, uintptr_t native_display, int x, int y,
	int desktop_width, int desktop_height)
{
	if ((input == NULL) || (input->mutex == NULL))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		input->native_window = native_window;
		input->native_display = native_display;
		input->window_x = x;
		input->window_y = y;
		input->desktop_width = desktop_width;
		input->desktop_height = desktop_height;
	}
	fb_MutexUnlock(input->mutex);
}

void fb_gfx3_input_platform_window_moved(FB_GFX3_INPUT_STATE *input,
	int x, int y)
{
	if ((input == NULL) || (input->mutex == NULL))
		return;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		input->window_x = x;
		input->window_y = y;
	}
	fb_MutexUnlock(input->mutex);
}

int fb_gfx3_input_platform_take_window_request(FB_GFX3_INPUT_STATE *input,
	FB_GFX3_WINDOW_REQUEST *request)
{
	int available = FALSE;

	if ((input == NULL) || (request == NULL) || (input->mutex == NULL))
		return FALSE;
	fb_MutexLock(input->mutex);
	if (input->initialized && (input->window_request.flags != 0)) {
		*request = input->window_request;
		input->window_request.flags = 0;
		available = TRUE;
	}
	fb_MutexUnlock(input->mutex);
	return available;
}

int fb_gfx3_input_window_snapshot(FB_GFX3_INPUT_STATE *input,
	uintptr_t *native_window, uintptr_t *native_display, int *x, int *y,
	int *desktop_width, int *desktop_height)
{
	int result = FB_GFX3_INVALID;

	if ((input == NULL) || (input->mutex == NULL))
		return result;
	fb_MutexLock(input->mutex);
	if (input->initialized) {
		if (native_window != NULL)
			*native_window = input->native_window;
		if (native_display != NULL)
			*native_display = input->native_display;
		if (x != NULL)
			*x = input->window_x;
		if (y != NULL)
			*y = input->window_y;
		if (desktop_width != NULL)
			*desktop_width = input->desktop_width;
		if (desktop_height != NULL)
			*desktop_height = input->desktop_height;
		result = FB_GFX3_OK;
	}
	fb_MutexUnlock(input->mutex);
	return result;
}

int fb_gfx3_input_request_window_position(FB_GFX3_INPUT_STATE *input,
	int x, int y)
{
	if ((input == NULL) || (input->mutex == NULL))
		return FB_GFX3_INVALID;
	fb_MutexLock(input->mutex);
	if (!input->initialized) {
		fb_MutexUnlock(input->mutex);
		return FB_GFX3_INVALID;
	}
	if (x == INT_MIN)
		x = input->window_x;
	if (y == INT_MIN)
		y = input->window_y;
	input->window_request.flags |= FB_GFX3_WINDOW_REQUEST_POSITION;
	input->window_request.x = x;
	input->window_request.y = y;
	fb_MutexUnlock(input->mutex);
	return FB_GFX3_OK;
}

/* end of gfx3_input.c */
