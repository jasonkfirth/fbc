/*
 * Project: FreeBASIC Linux graphics tests
 * File: wayland-text-commit.c
 * Purpose: Check text-input commits reach the legacy key and event queues.
 * Responsibilities: Exercise text, Enter, deletion and invalid UTF-8 locally.
 * This file intentionally does NOT connect to a compositor or inject input.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/wayland-text-commit.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/wayland-text-commit
 *   /tmp/wayland-text-commit
 */
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"

static int keys[320];
static EVENT events[640];
static size_t key_count;
static size_t event_count;
FBGFX *__fb_gfx;

int fb_hScancodeToExtendedKey(int scancode)
{
	return scancode == SC_ENTER ? '\r' : 0;
}

void fb_hPostKey(int key)
{
	if (key_count < sizeof(keys) / sizeof(keys[0]))
		keys[key_count++] = key;
}

void fb_hPostEvent(EVENT *event)
{
	if (event_count < sizeof(events) / sizeof(events[0]))
		events[event_count++] = *event;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "wayland-text-commit: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static uint32_t mock_xkb_update(struct xkb_state *state, uint32_t key,
	uint32_t direction)
{
	(void)state;
	(void)key;
	(void)direction;
	return 0;
}

static int mock_xkb_text(struct xkb_state *state, uint32_t key, char *text,
	size_t size)
{
	(void)state;
	if ((key == 38) && (size >= 4)) {
		memcpy(text, "\xE2\x82\xAC", 4);
		return 3;
	}
	if ((key != 179 && key != 141) || size < 2)
		return 0;
	text[0] = key == 179 ? '\r' : 'a';
	text[1] = '\0';
	return 1;
}

static uint32_t mock_xkb_symbol(struct xkb_state *state, uint32_t key)
{
	(void)state;
	return key == 179 ? 0xFF0D : (key == 141 ? 'a' : 0);
}

int main(void)
{
	FBGFX gfx;
	FB_WAYLAND_WINDOW text_window;
	char key_state[128];
	char long_text[1101];
	fb_wayland_post_text("abc\n\b");
	require(key_count == 5 && keys[0] == 'a' && keys[1] == 'b' &&
		keys[2] == 'c' && keys[3] == '\r' && keys[4] == '\b',
		"committed text did not reach INKEY in order");
	require(event_count == 10 && events[6].type == EVENT_KEY_PRESS &&
		events[6].scancode == SC_ENTER && events[6].ascii == '\r' &&
		events[8].scancode == SC_BACKSPACE,
		"committed control characters did not produce key events");

	fb_wayland_post_delete(2);
	require(key_count == 7 && keys[5] == '\b' && keys[6] == '\b' &&
		event_count == 14 && events[10].scancode == SC_BACKSPACE &&
		events[12].scancode == SC_BACKSPACE,
		"input-method deletion did not reach both queues");

	fb_wayland_post_text("A\xE2\x82\xAC");
	require(key_count == 8 && keys[7] == 'A' && event_count == 18 &&
		events[16].ascii == 0x20AC,
		"Unicode was mistaken for a legacy extended key");
	fb_wayland_post_text("\xE2\x82");
	require(key_count == 8 && event_count == 18,
		"truncated UTF-8 produced an input event");
	fb_wayland_text_v2_preedit(NULL, NULL, "abc", "abc");
	require(key_count == 8 && event_count == 18,
		"preedit fallback was submitted before commit_string");

	/* v3 changes take effect together at done, with deletion before text. */
	memset(&text_window, 0, sizeof(text_window));
	text_window.surface = (struct wl_surface *)(uintptr_t)1;
	text_window.text_input_entered = 1;
	text_window.keyboard_requested = 1;
	key_count = event_count = 0;
	fb_wayland_text_commit(&text_window, NULL, "x");
	fb_wayland_text_delete(&text_window, NULL, 2, 0);
	require(key_count == 0 && event_count == 0,
		"v3 edit reached the queues before done");
	fb_wayland_text_done(&text_window, NULL, 1);
	require(key_count == 3 && keys[0] == '\b' && keys[1] == '\b' &&
		keys[2] == 'x' && event_count == 6 &&
		events[0].scancode == SC_BACKSPACE && events[4].ascii == 'x',
		"v3 replacement was not applied in protocol order");
	fb_wayland_text_done(&text_window, NULL, 1);
	require(key_count == 3 && event_count == 6,
		"v3 done replayed an earlier edit");

	key_count = event_count = 0;
	fb_wayland_text_commit(&text_window, NULL, "old");
	fb_wayland_text_commit(&text_window, NULL, "new");
	fb_wayland_text_done(&text_window, NULL, 2);
	require(key_count == 3 && keys[0] == 'n' && keys[1] == 'e' &&
		keys[2] == 'w', "v3 pending commit did not replace its previous value");

	key_count = event_count = 0;
	fb_wayland_text_commit(&text_window, NULL, "discard");
	fb_wayland_text_commit(&text_window, NULL, NULL);
	fb_wayland_text_done(&text_window, NULL, 2);
	require(key_count == 0 && event_count == 0,
		"null v3 commit did not clear the pending value");
	fb_wayland_text_commit(&text_window, NULL, "hidden");
	text_window.keyboard_requested = 0;
	fb_wayland_text_done(&text_window, NULL, 2);
	text_window.keyboard_requested = 1;
	fb_wayland_text_done(&text_window, NULL, 2);
	require(key_count == 0 && event_count == 0,
		"a hidden editor received or replayed pending text");

	fb_wayland_text_commit(&text_window, NULL, "stale");
	fb_wayland_text_leave(&text_window, NULL, text_window.surface);
	fb_wayland_text_done(&text_window, NULL, 3);
	require(key_count == 0 && event_count == 0,
		"v3 edit survived a surface focus change");

	text_window.text_input_entered = 1;
	text_window.keyboard_requested = 1;
	memset(long_text, 'x', sizeof(long_text) - 1);
	long_text[sizeof(long_text) - 1] = '\0';
	fb_wayland_text_commit(&text_window, NULL, long_text);
	fb_wayland_text_done(&text_window, NULL, 4);
	require(key_count == 255 && event_count == 510,
		"long v3 commit exceeded the bounded input queues");

	/* The Harpia keyboard emits Return on evdev code 171. Its XKB keymap,
	 * rather than the numeric code, identifies this virtual key. */
	memset(&gfx, 0, sizeof(gfx));
	memset(key_state, 0, sizeof(key_state));
	memset(&fb_wayland_window, 0, sizeof(fb_wayland_window));
	gfx.key = key_state;
	__fb_gfx = &gfx;
	fb_wayland_window.keyboard_focused = 1;
	fb_wayland_window.xkb_state = (struct xkb_state *)(uintptr_t)1;
	fb_wayland_window.xkb.state_update_key = mock_xkb_update;
	fb_wayland_window.xkb.state_key_get_utf8 = mock_xkb_text;
	fb_wayland_window.xkb.state_key_get_one_sym = mock_xkb_symbol;
	key_count = 0;
	event_count = 0;
	fb_wayland_keyboard_key(&fb_wayland_window, NULL, 0, 0, 171, 1);
	require(key_count == 1 && keys[0] == '\r' && key_state[SC_ENTER] &&
		event_count == 1 && events[0].scancode == SC_ENTER &&
		events[0].type == EVENT_KEY_PRESS,
		"virtual Return did not produce a press");
	fb_wayland_keyboard_key(&fb_wayland_window, NULL, 0, 0, 171, 0);
	require(!key_state[SC_ENTER] && event_count == 2 &&
		events[1].scancode == SC_ENTER && events[1].type == EVENT_KEY_RELEASE,
		"virtual Return did not release its mapped scan code");

	/* After a focus switch Harpia's virtual keyboard assigns 'a' to evdev 133,
	 * which has no corresponding legacy FreeBASIC scan code. */
	key_count = event_count = 0;
	fb_wayland_window.repeat_interval_ms = 1;
	fb_wayland_window.repeat_delay_ms = 0;
	fb_wayland_keyboard_key(&fb_wayland_window, NULL, 0, 0, 133, 1);
	fb_wayland_keyboard_repeat();
	fb_wayland_keyboard_key(&fb_wayland_window, NULL, 0, 0, 133, 0);
	fb_wayland_keyboard_repeat();
	require(key_count == 2 && keys[0] == 'a' && keys[1] == 'a' &&
		event_count == 4 &&
		events[0].type == EVENT_KEY_PRESS && events[0].ascii == 'a' &&
		events[1].type == EVENT_KEY_RELEASE,
		"an unmapped virtual key lost its XKB text or repeated after release");

	/* Physical key text above 255 belongs in SCREENEVENT, never INKEY. */
	key_count = event_count = 0;
	fb_wayland_keyboard_key(&fb_wayland_window, NULL, 0, 0, 30, 1);
	fb_wayland_keyboard_repeat();
	fb_wayland_keyboard_key(&fb_wayland_window, NULL, 0, 0, 30, 0);
	fb_wayland_keyboard_repeat();
	require(key_count == 0 && event_count == 3 &&
		events[0].type == EVENT_KEY_PRESS && events[0].ascii == 0x20AC &&
		events[1].type == EVENT_KEY_REPEAT && events[1].ascii == 0x20AC &&
		events[2].type == EVENT_KEY_RELEASE && !key_state[SC_A],
		"mapped Unicode key or repeat was lost or entered INKEY");

	puts("WAYLAND_TEXT_COMMIT_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-text-commit.c */
