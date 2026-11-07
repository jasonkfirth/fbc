/*
 * FreeBASIC graphics tests
 * ------------------------
 * File: wayland-text-region.c
 * Purpose: Check Wayland text-region updates without resetting the IME.
 * Responsibilities: Exercise v2 and v3 requests with a recording client ABI.
 * This file intentionally does not connect to a compositor or inject input.
 *
 * Build on Linux with:
 *     cc -std=gnu11 -ffunction-sections -fdata-sections -pthread
 *        tests/gfx/wayland-text-region.c
 *        src/gfxlib2/drivers/linux/fb_wayland_webos.c
 *        -Wl,--gc-sections -ldl -lm
 */

#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"

static uint32_t recorded_requests[16];
static size_t recorded_count;
static int flush_count;

static void record_request(struct wl_proxy *proxy, uint32_t opcode, ...)
{
	(void)proxy;
	if (recorded_count < sizeof(recorded_requests) / sizeof(recorded_requests[0]))
		recorded_requests[recorded_count++] = opcode;
}

static int record_flush(struct wl_display *display)
{
	(void)display;
	flush_count++;
	return 0;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "wayland-text-region: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void reset_window(int version)
{
	static const FB_WAYLAND_API api = {
		.display_flush = record_flush,
		.proxy_marshal = record_request
	};

	memset(&fb_wayland_window, 0, sizeof(fb_wayland_window));
	fb_wayland_window.api = &api;
	fb_wayland_window.display = (struct wl_display *)(uintptr_t)1;
	fb_wayland_window.surface = (struct wl_surface *)(uintptr_t)2;
	if (version == 3)
		fb_wayland_window.text_input = (struct zwp_text_input_v3 *)(uintptr_t)3;
	else
		fb_wayland_window.text_input_v2 = (struct zwp_text_input_v2 *)(uintptr_t)4;
	fb_wayland_window.active = 1;
	fb_wayland_window.text_input_entered = 1;
	fb_wayland_window.keyboard_requested = 1;
	fb_wayland_window.width = 360;
	fb_wayland_window.height = 640;
	fb_wayland_window.region_x = 10;
	fb_wayland_window.region_y = 20;
	fb_wayland_window.region_width = 30;
	fb_wayland_window.region_height = 40;
	fb_wayland_remember_sent_region(&fb_wayland_window);
	recorded_count = 0;
	flush_count = 0;
}

int main(void)
{
	int version;
	int visible;

	/* omaGUI clears its registrations before publishing the focused field. */
	for (version = 2; version <= 3; version++) {
		reset_window(version);
		require(fb_hWaylandSetKeyboard(1) == 0, "initial show failed");
		recorded_count = 0;
		flush_count = 0;
		require(fb_hWaylandClearTextInputRegions() == 0,
			"region clear failed");
		require(fb_hWaylandSetTextInputRegion(10, 20, 30, 40) == 0,
			"restored region rejected");
		require(recorded_count == 0 && flush_count == 0,
			"clear and restore resent the compositor's unchanged region");
		require(fb_hWaylandSetTextInputRegion(11, 20, 30, 40) == 0 &&
			recorded_count == 2 && flush_count == 1,
			"changed region was not sent after clear and restore");
		recorded_count = 0;
		flush_count = 0;
		require(fb_hWaylandClearTextInputRegions() == 0 &&
			fb_hWaylandSetTextInputRegion(0, 0, 1, 1) == 0 &&
			recorded_count == 2 && flush_count == 1,
			"default-sized region did not replace the previously sent region");
	}

	/* A focused editor remains focused when the surface temporarily leaves. */
	for (version = 2; version <= 3; version++) {
		reset_window(version);
		if (version == 3)
			fb_wayland_text_leave(&fb_wayland_window,
				fb_wayland_window.text_input, fb_wayland_window.surface);
		else
			fb_wayland_text_v2_leave(&fb_wayland_window,
				fb_wayland_window.text_input_v2, 1, fb_wayland_window.surface);
		require(!fb_wayland_window.text_input_entered &&
			fb_wayland_window.keyboard_requested,
			"surface leave lost the focused editor's keyboard request");
		visible = -2;
		require(fb_hWaylandGetKeyboardState(&visible) == 1 && visible == 0,
			"unfocused surface reported a visible or unknown keyboard");
		recorded_count = 0;
		flush_count = 0;
		if (version == 3)
			fb_wayland_text_enter(&fb_wayland_window,
				fb_wayland_window.text_input, fb_wayland_window.surface);
		else
			fb_wayland_text_v2_enter(&fb_wayland_window,
				fb_wayland_window.text_input_v2, 2, fb_wayland_window.surface);
		require(recorded_count == (version == 3 ? 5u : 4u) &&
			flush_count == (version == 3 ? 2 : 1),
			"surface re-entry did not restore the focused keyboard");
		if (version == 3)
			require(recorded_requests[0] == FB_WL_TEXT_INPUT_DISABLE &&
				recorded_requests[1] == FB_WL_TEXT_INPUT_COMMIT &&
				recorded_requests[2] == FB_WL_TEXT_INPUT_ENABLE &&
				recorded_requests[4] == FB_WL_TEXT_INPUT_COMMIT,
				"v3 focus re-entry did not reset the old input state");

		if (version == 3)
			fb_wayland_text_leave(&fb_wayland_window,
				fb_wayland_window.text_input, fb_wayland_window.surface);
		else
			fb_wayland_text_v2_leave(&fb_wayland_window,
				fb_wayland_window.text_input_v2, 3, fb_wayland_window.surface);
		require(fb_hWaylandSetKeyboard(0) == -1 &&
			!fb_wayland_window.keyboard_requested,
			"hide while unfocused did not clear the keyboard request");
		recorded_count = 0;
		if (version == 3)
			fb_wayland_text_enter(&fb_wayland_window,
				fb_wayland_window.text_input, fb_wayland_window.surface);
		else
			fb_wayland_text_v2_enter(&fb_wayland_window,
				fb_wayland_window.text_input_v2, 4, fb_wayland_window.surface);
		require(recorded_count == 0,
			"surface re-entry ignored a hide request made while unfocused");
	}

	reset_window(3);
	require(fb_hWaylandSetTextInputRegion(10, 20, 30, 40) == 0,
		"v3 unchanged region rejected");
	require(recorded_count == 0 && flush_count == 0,
		"v3 unchanged region sent requests");
	require(fb_hWaylandSetTextInputRegion(11, 20, 30, 40) == 0,
		"v3 changed region rejected");
	require(recorded_count == 2 &&
		recorded_requests[0] == FB_WL_TEXT_INPUT_SET_CURSOR_RECTANGLE &&
		recorded_requests[1] == FB_WL_TEXT_INPUT_COMMIT &&
		flush_count == 1, "v3 region update reset the input context");

	reset_window(2);
	require(fb_hWaylandSetTextInputRegion(11, 20, 30, 40) == 0,
		"v2 changed region rejected");
	require(recorded_count == 2 &&
		recorded_requests[0] == FB_WL_TEXT_INPUT_V2_SET_CURSOR_RECTANGLE &&
		recorded_requests[1] == FB_WL_TEXT_INPUT_V2_UPDATE_STATE &&
		flush_count == 1, "v2 region update reopened the input panel");

	reset_window(3);
	fb_wayland_window.keyboard_requested = 0;
	require(fb_hWaylandSetTextInputRegion(11, 20, 30, 40) == 0,
		"hidden input region rejected");
	require(recorded_count == 0, "hidden input sent requests");
	require(fb_hWaylandSetKeyboard(1) == 0, "show request failed");
	require(recorded_count == 3 &&
		recorded_requests[0] == FB_WL_TEXT_INPUT_ENABLE &&
		recorded_requests[1] == FB_WL_TEXT_INPUT_SET_CURSOR_RECTANGLE &&
		recorded_requests[2] == FB_WL_TEXT_INPUT_COMMIT,
		"show did not publish the stored region");

	reset_window(3);
	require(fb_hWaylandSetTextInputRegion(350, 20, 30, 40) == -1,
		"out-of-bounds region accepted");
	require(recorded_count == 0, "invalid region sent requests");

	/* Protocol callbacks run with the driver mutex already held. */
	reset_window(3);
	fb_wayland_window.text_input_entered = 0;
	alarm(5);
	pthread_mutex_lock(&fb_wayland_mutex);
	fb_wayland_text_enter(&fb_wayland_window, fb_wayland_window.text_input,
		fb_wayland_window.surface);
	pthread_mutex_unlock(&fb_wayland_mutex);
	alarm(0);
	require(recorded_count == 5 && flush_count == 2,
		"v3 focus callback did not restore the requested keyboard");

	reset_window(2);
	fb_wayland_window.text_input_entered = 0;
	alarm(5);
	pthread_mutex_lock(&fb_wayland_mutex);
	fb_wayland_text_v2_enter(&fb_wayland_window, fb_wayland_window.text_input_v2,
		7, fb_wayland_window.surface);
	pthread_mutex_unlock(&fb_wayland_mutex);
	alarm(0);
	require(recorded_count == 4 && flush_count == 1,
		"v2 focus callback did not restore the requested keyboard");

	puts("wayland-text-region: passed");
	return EXIT_SUCCESS;
}

/* end of wayland-text-region.c */
