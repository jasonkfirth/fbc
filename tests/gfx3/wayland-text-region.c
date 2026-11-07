/*
 * FreeBASIC gfxlib3 tests
 * -----------------------
 * File: wayland-text-region.c
 * Purpose: Check queued Wayland text-region and keyboard requests.
 * Responsibilities: Record v2 and v3 messages and their surface arguments.
 * This file intentionally does not connect to a compositor or inject input.
 *
 * Build on Linux with:
 *     cc -std=gnu11 -ffunction-sections -fdata-sections -pthread
 *        tests/gfx3/wayland-text-region.c -Wl,--gc-sections -ldl -lm
 */

#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <stdio.h>
#include "../../src/gfxlib3/drivers/linux/gfx3_platform_wayland.c"

static uint32_t recorded_requests[16];
static size_t recorded_count;
static int flush_count;
static int protocol_version;
static struct wl_proxy *recorded_surface;
static int region_pending;
static int test_x, test_y, test_width, test_height;
static int keyboard_available, keyboard_visible;

static void record_request(struct wl_proxy *proxy, uint32_t opcode, ...)
{
	va_list args;
	(void)proxy;
	if (recorded_count < sizeof(recorded_requests) / sizeof(recorded_requests[0]))
		recorded_requests[recorded_count++] = opcode;
	if (protocol_version == 2 &&
		(opcode == FB_GFX3_WL_TEXT_INPUT_V2_ENABLE ||
		 opcode == FB_GFX3_WL_TEXT_INPUT_V2_DISABLE)) {
		va_start(args, opcode);
		recorded_surface = va_arg(args, struct wl_proxy *);
		va_end(args);
	}
}

static int record_flush(struct wl_display *display)
{
	(void)display;
	flush_count++;
	return 0;
}

int fb_gfx3_input_take_soft_keyboard_request(FB_GFX3_INPUT_STATE *input,
	int *visible)
{
	(void)input;
	(void)visible;
	return 0;
}

int fb_gfx3_input_take_text_region(FB_GFX3_INPUT_STATE *input,
	int *x, int *y, int *width, int *height)
{
	(void)input;
	if (!region_pending)
		return 0;
	region_pending = 0;
	*x = test_x;
	*y = test_y;
	*width = test_width;
	*height = test_height;
	return 1;
}

void fb_gfx3_input_platform_keyboard_state(FB_GFX3_INPUT_STATE *input,
	int available, int visible)
{
	(void)input;
	keyboard_available = available;
	keyboard_visible = visible;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "gfx3-wayland-text-region: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void reset_platform(FB_GFX3_PLATFORM_WAYLAND *platform, int version)
{
	static const FB_WAYLAND_API api = {
		.display_flush = record_flush,
		.proxy_marshal = record_request
	};
	static FB_GFX3_INPUT_STATE input;

	memset(platform, 0, sizeof(*platform));
	platform->api = &api;
	platform->display = (struct wl_display *)(uintptr_t)1;
	platform->surface = (struct wl_proxy *)(uintptr_t)2;
	platform->input = &input;
	if (version == 3)
		platform->text_input = (struct wl_proxy *)(uintptr_t)3;
	else
		platform->text_input_v2 = (struct wl_proxy *)(uintptr_t)4;
	platform->text_input_entered = 1;
	platform->keyboard_requested = 1;
	platform->region_x = 10;
	platform->region_y = 20;
	platform->region_width = 30;
	platform->region_height = 40;
	protocol_version = version;
	recorded_count = 0;
	flush_count = 0;
	recorded_surface = NULL;
	region_pending = 1;
	test_x = 11;
	test_y = 20;
	test_width = 30;
	test_height = 40;
}

int main(void)
{
	FB_GFX3_PLATFORM_WAYLAND platform;

	/* The compositor cannot show this client's v3 panel before focus enters. */
	reset_platform(&platform, 3);
	platform.text_input_entered = 0;
	platform_wayland_publish_keyboard_state(&platform);
	require(keyboard_available == 1 && keyboard_visible == 0,
		"v3 keyboard visibility was unknown before surface focus");
	platform.text_input_entered = 1;
	platform_wayland_publish_keyboard_state(&platform);
	require(keyboard_visible == -2,
		"v3 keyboard visibility was claimed after an enable request");
	platform.keyboard_requested = 0;
	platform_wayland_publish_keyboard_state(&platform);
	require(keyboard_visible == 0,
		"v3 keyboard visibility was unknown with no enable request");
	reset_platform(&platform, 2);
	platform.text_input_entered = 0;
	platform_wayland_publish_keyboard_state(&platform);
	require(keyboard_available == 1 && keyboard_visible == -2,
		"v2 keyboard visibility was inferred before a panel event");
	platform_wayland_text_v2_panel(&platform, platform.text_input_v2,
		1, 0, 0, 0, 0);
	require(keyboard_visible == -1,
		"v2 keyboard visibility ignored a panel event");

	reset_platform(&platform, 3);
	platform_wayland_apply_queued_input(&platform);
	require(recorded_count == 2 &&
		recorded_requests[0] == FB_GFX3_WL_TEXT_INPUT_SET_CURSOR_RECTANGLE &&
		recorded_requests[1] == FB_GFX3_WL_TEXT_INPUT_COMMIT &&
		flush_count == 1, "v3 region update reset the input context");
	recorded_count = 0;
	flush_count = 0;
	region_pending = 1;
	platform_wayland_apply_queued_input(&platform);
	require(recorded_count == 0 && flush_count == 0,
		"unchanged v3 region sent requests");

	reset_platform(&platform, 2);
	platform_wayland_apply_queued_input(&platform);
	require(recorded_count == 2 &&
		recorded_requests[0] == FB_GFX3_WL_TEXT_INPUT_V2_SET_CURSOR_RECTANGLE &&
		recorded_requests[1] == FB_GFX3_WL_TEXT_INPUT_V2_UPDATE_STATE &&
		flush_count == 1, "v2 region update reopened the input panel");
	recorded_count = 0;
	platform_wayland_apply_keyboard(&platform);
	require(recorded_surface == platform.surface, "v2 enable omitted its surface");
	platform.keyboard_requested = 0;
	recorded_surface = NULL;
	platform_wayland_apply_keyboard(&platform);
	require(recorded_surface == platform.surface, "v2 disable omitted its surface");

	reset_platform(&platform, 3);
	platform_wayland_text_v3_leave(&platform, platform.text_input, platform.surface);
	require(!platform.text_input_entered && platform.keyboard_requested,
		"surface leave lost the focused editor's keyboard request");
	recorded_count = 0;
	flush_count = 0;
	platform_wayland_text_v3_enter(&platform, platform.text_input, platform.surface);
	require(recorded_count == 6 && flush_count == 2 &&
		recorded_requests[0] == FB_GFX3_WL_TEXT_INPUT_DISABLE &&
		recorded_requests[1] == FB_GFX3_WL_TEXT_INPUT_COMMIT &&
		recorded_requests[2] == FB_GFX3_WL_TEXT_INPUT_ENABLE &&
		recorded_requests[5] == FB_GFX3_WL_TEXT_INPUT_COMMIT,
		"v3 focus re-entry did not reset the old input state");
	require(keyboard_available == 1 && keyboard_visible == -2,
		"v3 keyboard visibility was claimed after focus re-entry");
	platform_wayland_text_v3_leave(&platform, platform.text_input, platform.surface);
	platform.keyboard_requested = 0;
	recorded_count = 0;
	flush_count = 0;
	platform_wayland_text_v3_enter(&platform, platform.text_input, platform.surface);
	require(recorded_count == 0 && flush_count == 0,
		"a keyboard hidden while unfocused reopened on surface re-entry");

	puts("gfx3-wayland-text-region: passed");
	return EXIT_SUCCESS;
}

/* end of wayland-text-region.c */
