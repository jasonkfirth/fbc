/*
 * Project: FreeBASIC gfxlib3 Linux tests
 * File: wayland-text-commit.c
 * Purpose: Check committed Wayland text reaches both input queues.
 * Responsibilities: Exercise composition, Unicode, control keys, and bounds.
 * This file intentionally does NOT connect to a compositor or inject input.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx3/wayland-text-commit.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/gfx3-wayland-text-commit
 *   /tmp/gfx3-wayland-text-commit
 */
#include "../../src/gfxlib3/drivers/linux/gfx3_platform_wayland.c"
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
		fprintf(stderr, "gfx3-wayland-text-commit: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	FB_GFX3_PLATFORM_WAYLAND platform;
	FB_GFX3_INPUT_STATE input;
	char long_text[301];

	memset(&platform, 0, sizeof(platform));
	memset(&input, 0, sizeof(input));
	input.mutex = (FBMUTEX *)(uintptr_t)1;
	input.initialized = TRUE;
	platform.input = &input;
	platform.surface = (struct wl_proxy *)(uintptr_t)1;
	platform.text_input_entered = TRUE;
	platform.keyboard_requested = TRUE;

	platform_wayland_text_v2_preedit(&platform, NULL, "abc", "abc");
	require(input.key_tail == 0 && input.event_tail == 0,
		"preedit fallback was submitted before commit_string");
	platform_wayland_text_v2_commit(&platform, NULL, "abc\n\b\xE2\x82\xAC");
	require(input.key_tail == 5 && input.key_buffer[0] == 'a' &&
		input.key_buffer[1] == 'b' && input.key_buffer[2] == 'c' &&
		input.key_buffer[3] == '\r' && input.key_buffer[4] == '\b',
		"committed bytes did not reach INKEY in order");
	require(input.event_tail == 12 &&
		input.event_queue[6].type == EVENT_KEY_PRESS &&
		input.event_queue[6].scancode == SC_ENTER &&
		input.event_queue[8].scancode == SC_BACKSPACE &&
		input.event_queue[10].ascii == 0x20AC,
		"committed control or Unicode event was lost");
	platform_wayland_text_v3_commit(&platform, NULL, "x");
	platform_wayland_text_v3_delete(&platform, NULL, 2, 0);
	require(input.key_tail == 5 && input.event_tail == 12,
		"v3 edit reached the queues before done");
	platform_wayland_text_v3_done(&platform, NULL, 1);
	require(input.key_tail == 8 && input.key_buffer[5] == '\b' &&
		input.key_buffer[6] == '\b' && input.key_buffer[7] == 'x' &&
		input.event_tail == 18 && input.event_queue[12].scancode == SC_BACKSPACE &&
		input.event_queue[16].ascii == 'x',
		"v3 replacement was not applied in protocol order");
	platform_wayland_text_v3_done(&platform, NULL, 1);
	require(input.key_tail == 8 && input.event_tail == 18,
		"v3 done replayed an earlier edit");
	platform_wayland_emit_text(&platform, "\xE2\x82");
	require(input.key_tail == 8 && input.event_tail == 18,
		"truncated UTF-8 produced an input event");

	memset(&input.key_buffer, 0, sizeof(input.key_buffer));
	input.key_head = input.key_tail = 0;
	input.event_head = input.event_tail = 0;
	platform_wayland_text_v3_commit(&platform, NULL, "old");
	platform_wayland_text_v3_commit(&platform, NULL, "new");
	platform_wayland_text_v3_done(&platform, NULL, 2);
	require(input.key_tail == 3 && input.key_buffer[0] == 'n' &&
		input.key_buffer[1] == 'e' && input.key_buffer[2] == 'w',
		"v3 pending commit did not replace its previous value");

	input.key_head = input.key_tail = 0;
	input.event_head = input.event_tail = 0;
	platform_wayland_text_v3_commit(&platform, NULL, "discard");
	platform_wayland_text_v3_commit(&platform, NULL, NULL);
	platform_wayland_text_v3_done(&platform, NULL, 2);
	require(input.key_tail == 0 && input.event_tail == 0,
		"null v3 commit did not clear the pending value");
	platform_wayland_text_v3_commit(&platform, NULL, "hidden");
	platform.keyboard_requested = FALSE;
	platform_wayland_text_v3_done(&platform, NULL, 2);
	platform.keyboard_requested = TRUE;
	platform_wayland_text_v3_done(&platform, NULL, 2);
	require(input.key_tail == 0 && input.event_tail == 0,
		"a hidden editor received or replayed pending text");

	platform_wayland_text_v3_commit(&platform, NULL, "stale");
	platform_wayland_text_v3_leave(&platform, NULL, platform.surface);
	platform_wayland_text_v3_done(&platform, NULL, 3);
	require(input.key_tail == 0 && input.event_tail == 0,
		"v3 edit survived a surface focus change");

	platform.text_input_entered = TRUE;
	platform.keyboard_requested = TRUE;
	memset(long_text, 'x', sizeof(long_text) - 1);
	long_text[sizeof(long_text) - 1] = '\0';
	platform_wayland_text_v3_commit(&platform, NULL, long_text);
	platform_wayland_text_v3_done(&platform, NULL, 4);
	require(input.key_head == 0 && input.key_tail == 255 &&
		input.event_head == 0 && input.event_tail == 510,
		"a long commit exceeded the empty bounded queues");

	puts("GFX3_WAYLAND_TEXT_COMMIT_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-text-commit.c */
