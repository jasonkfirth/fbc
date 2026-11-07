/*
 * Project: FreeBASIC gfxlib3 Linux tests
 * File: wayland-virtual-key.c
 * Purpose: Verify XKB control keys from a virtual keyboard.
 * Responsibilities: Check mapped press and release events and key input.
 * This file intentionally does NOT connect to a compositor or inject input.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx3/wayland-virtual-key.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/gfx3-wayland-virtual-key
 *   /tmp/gfx3-wayland-virtual-key
 */
#include "../../src/gfxlib3/drivers/linux/gfx3_platform_wayland.c"

static int event_types[4];
static int event_scancodes[4];
static int event_count;
static int posted_character;
static uint32_t posted_text;
static unsigned int posted_text_count;

int fb_hScancodeToExtendedKey(int scancode)
{
	return scancode == SC_ENTER ? '\r' : 0;
}

void fb_gfx3_input_platform_key(FB_GFX3_INPUT_STATE *input, int type,
	int scancode, int ascii)
{
	(void)input;
	(void)ascii;
	if (event_count < 4) {
		event_types[event_count] = type;
		event_scancodes[event_count++] = scancode;
	}
}

void fb_gfx3_input_platform_character(FB_GFX3_INPUT_STATE *input, int key,
	uint32_t repeat_count)
{
	(void)input;
	if (repeat_count == 1)
		posted_character = key;
}

void fb_gfx3_input_platform_text(FB_GFX3_INPUT_STATE *input, uint32_t codepoint)
{
	(void)input;
	posted_text = codepoint;
	posted_text_count++;
}

static uint32_t mock_update(struct xkb_state *state, uint32_t key,
	uint32_t direction)
{
	(void)state;
	(void)key;
	(void)direction;
	return 0;
}

static int mock_text(struct xkb_state *state, uint32_t key, char *text,
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

static uint32_t mock_symbol(struct xkb_state *state, uint32_t key)
{
	(void)state;
	return key == 179 ? 0xFF0D : (key == 141 ? 'a' : 0);
}

int main(void)
{
	FB_GFX3_PLATFORM_WAYLAND platform;
	FB_GFX3_INPUT_STATE input;
	memset(&platform, 0, sizeof(platform));
	memset(&input, 0, sizeof(input));
	platform.input = &input;
	platform.xkb_state = (struct xkb_state *)(uintptr_t)1;
	platform.xkb.state_update_key = mock_update;
	platform.xkb.state_key_get_utf8 = mock_text;
	platform.xkb.state_key_get_one_sym = mock_symbol;
	platform_wayland_key_event(&platform, 171, 1, EVENT_KEY_PRESS);
	platform_wayland_key_event(&platform, 171, 0, EVENT_KEY_RELEASE);
	if (event_count != 2 || event_types[0] != EVENT_KEY_PRESS ||
	    event_types[1] != EVENT_KEY_RELEASE ||
	    event_scancodes[0] != SC_ENTER || event_scancodes[1] != SC_ENTER ||
	    posted_character != '\r' || platform.key_down[SC_ENTER] != 0) {
		fputs("gfx3-wayland-virtual-key: Return was not delivered\n", stderr);
		return EXIT_FAILURE;
	}
	event_count = 0;
	posted_text = 0;
	platform.repeat_interval_ms = 1u;
	platform.repeat_delay_ms = 0u;
	platform_wayland_key_event(&platform, 133, 1, EVENT_KEY_PRESS);
	platform_wayland_repeat_tick(&platform);
	platform_wayland_key_event(&platform, 133, 0, EVENT_KEY_RELEASE);
	platform_wayland_repeat_tick(&platform);
	if (posted_text != 'a' || posted_text_count != 2u || event_count != 0) {
		fputs("gfx3-wayland-virtual-key: unmapped XKB text or repeat failed\n", stderr);
		return EXIT_FAILURE;
	}
	platform_wayland_key_event(&platform, 133, 1, EVENT_KEY_PRESS);
	platform_wayland_keyboard_repeat_info(&platform, NULL, 0, 0);
	platform_wayland_keyboard_repeat_info(&platform, NULL, 100, 0);
	platform_wayland_repeat_tick(&platform);
	platform_wayland_key_event(&platform, 133, 0, EVENT_KEY_RELEASE);
	if (posted_text_count != 3u) {
		fputs("gfx3-wayland-virtual-key: disabled repeat resumed a stale key\n", stderr);
		return EXIT_FAILURE;
	}
	/* A mapped physical key can produce a Unicode scalar, including repeats. */
	event_count = 0;
	posted_character = 0;
	posted_text = 0;
	posted_text_count = 0;
	platform.repeat_interval_ms = 1u;
	platform.repeat_delay_ms = 0u;
	platform_wayland_key_event(&platform, 30, 1, EVENT_KEY_PRESS);
	platform_wayland_repeat_tick(&platform);
	platform_wayland_key_event(&platform, 30, 0, EVENT_KEY_RELEASE);
	platform_wayland_repeat_tick(&platform);
	if (event_count != 3 || event_types[0] != EVENT_KEY_PRESS ||
	    event_types[1] != EVENT_KEY_REPEAT || event_types[2] != EVENT_KEY_RELEASE ||
	    event_scancodes[0] != SC_A || posted_character != 0 ||
	    posted_text != 0x20ACu || posted_text_count != 2u ||
	    platform.key_down[SC_A] != 0) {
		fputs("gfx3-wayland-virtual-key: mapped Unicode key or repeat failed\n", stderr);
		return EXIT_FAILURE;
	}
	puts("GFX3_WAYLAND_VIRTUAL_KEY_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-virtual-key.c */
