/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_input_api.c

    Purpose:

        Connect the common gfxlib3 input state to FreeBASIC's established
        keyboard, mouse, and SCREENEVENT entry points.

    Responsibilities:

        - install the runtime input hooks owned by an active graphics mode
        - implement INKEY, GETKEY, KEYHIT, MULTIKEY, and mouse behavior
		- keep graphics-aware SLEEP responsive to pending keyboard input
        - preserve SCREENEVENT peek and consume semantics
		- read snapshots without synchronizing unrelated renderer work

    This file intentionally does NOT contain:

        - native window messages or virtual-key translation
        - input queue storage and platform producer algorithms
        - graphical console drawing or line editing
*/

#include "gfx3_api_internal.h"
#include "gfx3_console.h"
#include "gfx3_input.h"

/* ------------------------------------------------------------------------- */
/* Active mode and key queue helpers                                         */
/* ------------------------------------------------------------------------- */

static FB_GFX3_INPUT_STATE *input_active_locked(FB_GFX3_DRAW_STATE **state)
{
	FB_GFX3_DRAW_STATE *current = fb_gfx3_api_get_draw_state_locked();

	if (state != NULL)
		*state = current;
	if ((current == NULL) || (current->mode == NULL))
		return NULL;
	return &current->mode->input;
}

static int input_pop_key(FB_GFX3_INPUT_STATE *input)
{
	int key = 0;

	if ((input == NULL) || (input->mutex == NULL))
		return 0;
	fb_MutexLock(input->mutex);
	if (input->initialized && (input->key_head != input->key_tail)) {
		key = input->key_buffer[input->key_head];
		input->key_head = (input->key_head + 1u) &
			(FB_GFX3_INPUT_KEY_BUFFER_LENGTH - 1u);
	}
	fb_MutexUnlock(input->mutex);
	return key;
}

/* ------------------------------------------------------------------------- */
/* Keyboard hooks                                                            */
/* ------------------------------------------------------------------------- */

FBSTRING *fb_GfxInkey(void)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_INPUT_STATE *input;
	FBSTRING *result = &__fb_ctx.null_desc;
	int key;

	FB_GRAPHICS_LOCK();
	input = input_active_locked(&state);
	if (input != NULL) {
		key = input_pop_key(input);
		if (key != 0)
			result = fb_hMakeInkeyStr(key);
	}
	FB_GRAPHICS_UNLOCK();
	return result;
}

int fb_GfxGetkey(void)
{
	int key = 0;

	for (;;) {
		FB_GFX3_DRAW_STATE *state;
		FB_GFX3_INPUT_STATE *input;

		FB_GRAPHICS_LOCK();
		input = input_active_locked(&state);
		if (input == NULL) {
			FB_GRAPHICS_UNLOCK();
			break;
		}
		key = input_pop_key(input);
		FB_GRAPHICS_UNLOCK();
		if (key != 0)
			break;
		fb_Sleep(20);
	}
	return key;
}

int fb_GfxKeyHit(void)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_INPUT_STATE *input;
	int result = FALSE;

	FB_GRAPHICS_LOCK();
	input = input_active_locked(&state);
	if (input != NULL) {
		fb_MutexLock(input->mutex);
		result = input->initialized &&
			(input->key_head != input->key_tail);
		fb_MutexUnlock(input->mutex);
	}
	FB_GRAPHICS_UNLOCK();
	return result ? 1 : 0;
}

int fb_GfxMultikey(int scancode)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_INPUT_STATE *input;
	int result = FB_FALSE;

	FB_GRAPHICS_LOCK();
	input = input_active_locked(&state);
	/*
		MULTIKEY is an asynchronous state query. Platform callbacks publish
		key changes into the mutex-protected input snapshot independently of
		the renderer command queue. Flushing that queue for every queried key
		serialized an entire GLES frame dozens of times in games which sample
		held controls. Read the already-published snapshot directly.
	*/
	if ((input != NULL) && (scancode >= 0) &&
	    ((uint32_t)scancode < FB_GFX3_INPUT_KEY_COUNT)) {
		fb_MutexLock(input->mutex);
		result = input->key[scancode] ? FB_TRUE : FB_FALSE;
		fb_MutexUnlock(input->mutex);
	}
	FB_GRAPHICS_UNLOCK();
	return result;
}

/* ------------------------------------------------------------------------- */
/* Mouse hooks                                                               */
/* ------------------------------------------------------------------------- */

int fb_GfxGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_INPUT_STATE *input;
	int failed = TRUE;

	FB_GRAPHICS_LOCK();
	input = input_active_locked(&state);
	/* Mouse motion and buttons use the same asynchronous snapshot as keys. */
	if (input != NULL) {
		fb_MutexLock(input->mutex);
		if (input->initialized && input->focused && input->mouse_inside) {
			if (x != NULL)
				*x = input->mouse_x;
			if (y != NULL)
				*y = input->mouse_y;
			if (z != NULL)
				*z = input->mouse_z;
			if (buttons != NULL)
				*buttons = input->mouse_buttons;
			if (clip != NULL)
				*clip = input->mouse_clip;
			failed = FALSE;
		}
		fb_MutexUnlock(input->mutex);
	}
	FB_GRAPHICS_UNLOCK();
	if (failed) {
		if (x != NULL)
			*x = -1;
		if (y != NULL)
			*y = -1;
		if (z != NULL)
			*z = -1;
		if (buttons != NULL)
			*buttons = -1;
		if (clip != NULL)
			*clip = -1;
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxSetMouse(int x, int y, int cursor, int clip)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_INPUT_STATE *input;
	FB_GFX3_MOUSE_REQUEST *request;
	int result = FB_GFX3_INVALID;

	FB_GRAPHICS_LOCK();
	input = input_active_locked(&state);
	if ((input != NULL) && (input->mutex != NULL)) {
		fb_MutexLock(input->mutex);
		request = &input->mouse_request;
		if ((x != INT_MIN) || (y != INT_MIN)) {
			if (x == INT_MIN)
				x = input->mouse_x;
			if (y == INT_MIN)
				y = input->mouse_y;
			if (x < 0)
				x = 0;
			if (y < 0)
				y = 0;
			if (x >= (int)input->width)
				x = (int)input->width - 1;
			if (y >= (int)input->height)
				y = (int)input->height - 1;
			request->flags |= FB_GFX3_MOUSE_REQUEST_POSITION;
			request->x = x;
			request->y = y;
			input->mouse_x = x;
			input->mouse_y = y;
			input->mouse_inside = TRUE;
		}
		if (cursor >= 0) {
			request->flags |= FB_GFX3_MOUSE_REQUEST_CURSOR;
			request->cursor = cursor;
			input->cursor_visible = (cursor != 0);
		}
		if (clip >= 0) {
			request->flags |= FB_GFX3_MOUSE_REQUEST_CLIP;
			request->clip = clip;
			input->mouse_clip = (clip != 0);
		}
		fb_MutexUnlock(input->mutex);
		/*
			The renderer's 16 ms platform pump applies this request. The public
			snapshot was updated above, so an immediate GETMOUSE still observes the
			requested state without waiting for unrelated GPU commands.
		*/
		result = FB_GFX3_OK;
	}
	FB_GRAPHICS_UNLOCK();
	return fb_ErrorSetNum(fb_gfx3_api_runtime_error(result));
}

/* ------------------------------------------------------------------------- */
/* Event queue and hook ownership                                            */
/* ------------------------------------------------------------------------- */

FBCALL int fb_GfxEvent(EVENT *event)
{
	FB_GFX3_DRAW_STATE *state;
	FB_GFX3_INPUT_STATE *input;
	int available = FALSE;

	FB_GRAPHICS_LOCK();
	input = input_active_locked(&state);
	/*
		gfxlib2's SCREENEVENT reads the synchronized event queue; its window
		thread publishes native messages independently. gfxlib3 follows the same
		contract. Every backend execution pumps its window before GPU work, while
		the renderer's idle pump covers programs which are not drawing.

		Submitting a synchronous render-thread command for every empty query made
		ordinary game input loops hundreds of times more expensive than gfxlib2
		and introduced a graphics ordering boundary into an input-only API.
	*/
	if (input != NULL) {
		fb_MutexLock(input->mutex);
		if (input->initialized &&
		    (input->event_head != input->event_tail)) {
			available = TRUE;
			if (event != NULL) {
				*event = input->event_queue[input->event_head];
				input->event_head = (input->event_head + 1u) &
					(MAX_EVENTS - 1u);
			}
		}
		fb_MutexUnlock(input->mutex);
	}
	fb_gfx3_api_apply_pending_resize_locked(state);
	FB_GRAPHICS_UNLOCK();
	return available ? FB_TRUE : FB_FALSE;
}

int fb_GfxIsRedir(int is_input)
{
	(void)is_input;
	return FB_FALSE;
}

void fb_GfxSleep(int milliseconds)
{
	if (milliseconds == -1) {
		while (fb_GfxKeyHit() == 0)
			fb_Delay(50);
		return;
	}
	if (milliseconds >= 100) {
		while (milliseconds > 50) {
			if (fb_GfxKeyHit() != 0)
				return;
			fb_Delay(50);
			milliseconds -= 50;
		}
	}
	if (milliseconds == 0) {
		(void)fb_GfxKeyHit();
		fb_GfxViewUpdate();
	}
	if (milliseconds >= 0)
		fb_Delay(milliseconds);
}

void fb_gfx3_input_install_hooks_locked(void)
{
	__fb_ctx.hooks.inkeyproc = fb_GfxInkey;
	__fb_ctx.hooks.getkeyproc = fb_GfxGetkey;
	__fb_ctx.hooks.keyhitproc = fb_GfxKeyHit;
	__fb_ctx.hooks.multikeyproc = fb_GfxMultikey;
	__fb_ctx.hooks.getmouseproc = fb_GfxGetMouse;
	__fb_ctx.hooks.setmouseproc = fb_GfxSetMouse;
	__fb_ctx.hooks.sleepproc = fb_GfxSleep;
	__fb_ctx.hooks.isredirproc = fb_GfxIsRedir;
}

/* end of gfx3_input_api.c */
