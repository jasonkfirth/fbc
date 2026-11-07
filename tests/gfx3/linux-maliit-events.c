/*
 * Project: FreeBASIC gfxlib3 Linux tests
 * -------------------------------------
 *
 * File: linux-maliit-events.c
 * Purpose: Check Maliit text and editing keys reach both gfxlib3 queues.
 * Responsibilities: Distinguish Unicode text from encoded extended keys.
 * This file intentionally does NOT connect to a desktop or input method.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx3/linux-maliit-events.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/gfx3-linux-maliit-events
 *   /tmp/gfx3-linux-maliit-events
 */
#define DISABLE_X11
#include "../../src/gfxlib3/drivers/linux/gfx3_mobile.c"
#include "../../src/gfxlib3/core/gfx3_input.c"

FBCALL void fb_MutexLock(FBMUTEX *mutex)
{
	(void)mutex;
}

FBCALL void fb_MutexUnlock(FBMUTEX *mutex)
{
	(void)mutex;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "gfx3-linux-maliit-events: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	FB_GFX3_INPUT_STATE input;
	memset(&input, 0, sizeof(input));
	input.mutex = (FBMUTEX *)(uintptr_t)1;
	input.initialized = TRUE;
	gfx3_linux_maliit_post_key(&input, 'a', FB_LINUX_MALIIT_TEXT);
	gfx3_linux_maliit_post_key(&input, 0x4bffu, FB_LINUX_MALIIT_TEXT);
	gfx3_linux_maliit_post_key(&input, 0x4bffu, FB_LINUX_MALIIT_KEY);
	gfx3_linux_maliit_post_key(&input, '\r', FB_LINUX_MALIIT_KEY);
	gfx3_linux_maliit_post_key(&input, '\t', FB_LINUX_MALIIT_TEXT);
	require(input.key_tail == 4 && input.key_buffer[0] == 'a' &&
		input.key_buffer[1] == 0x4bff && input.key_buffer[2] == '\r' &&
		input.key_buffer[3] == '\t', "legacy keys differ");
	require(input.event_tail == 10, "missing press or release events");
	require(input.event_queue[0].type == EVENT_KEY_PRESS &&
		input.event_queue[0].ascii == 'a' &&
		input.event_queue[1].type == EVENT_KEY_RELEASE, "text event differs");
	require(input.event_queue[2].ascii == 0x4bff &&
		input.event_queue[2].scancode == 0, "Unicode was treated as a key");
	require(input.event_queue[4].ascii == 0 &&
		input.event_queue[4].scancode == SC_LEFT, "extended key was treated as text");
	require(input.event_queue[6].ascii == '\r' &&
		input.event_queue[6].scancode == SC_ENTER &&
		input.event_queue[8].ascii == '\t' &&
		input.event_queue[8].scancode == SC_TAB, "editing key scan codes differ");
	puts("GFX3_LINUX_MALIIT_EVENTS_PASS");
	return EXIT_SUCCESS;
}

/* end of linux-maliit-events.c */
