/*
 * Project: FreeBASIC gfxlib2 Linux tests
 * File: wayland-keyboard-reservation.c
 * Purpose: Qualify small keyboard reservations in maximized v3 surfaces.
 * Responsibilities: Exercise configure acknowledgement, request lifetime,
 * rotation and exclusion of floating or fullscreen window changes.
 * This file does not connect to a compositor or allocate native buffers.
 */
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"

static FBGFX context;
FBGFX *fb_gfx = &context;
static unsigned checks;

int fb_hWaylandWebosKeyboard(FB_WAYLAND_WEBOS *webos, int visible)
{
	(void)webos;
	(void)visible;
	return -1;
}

int fb_hWaylandWebosRegion(FB_WAYLAND_WEBOS *webos, int x, int y, int width, int height)
{
	(void)webos;
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	return -1;
}

void fb_hRequestResize(int width, int height)
{
	(void)width;
	(void)height;
}

int fb_hLinuxMaliitGetKeyboardOcclusion(int width, int height, int *top, int *overlap)
{
	(void)width;
	(void)height;
	(void)top;
	(void)overlap;
	return 0;
}

static void require(int condition, const char *message)
{
	checks++;
	if (!condition) {
		fprintf(stderr, "wayland-keyboard-reservation: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void acknowledge(struct wl_proxy *target, uint32_t opcode, ...)
{
	require(target == (struct wl_proxy *)fb_wayland_window.xdg_surface &&
		opcode == FB_WL_XDG_SURFACE_ACK_CONFIGURE, "unexpected native request");
}

static void configure(int width, int height, int maximized)
{
	uint32_t state = 1; /* xdg_toplevel.state.maximized */
	struct wl_array states = { maximized ? sizeof(state) : 0, sizeof(state), &state };
	fb_wayland_toplevel_configure(NULL, NULL, width, height, &states);
	fb_wayland_xdg_configure(NULL, fb_wayland_window.xdg_surface, 7);
	/* Complete the requested framebuffer transaction before acknowledging it. */
	fb_wayland_window.width = context.w = width;
	fb_wayland_window.height = context.h = height;
	require(fb_wayland_ack_configure(&fb_wayland_window), "matching buffer not acknowledged");
}

static int marker(int expected_height)
{
	int top = -7, overlap = -9;
	int result = fb_hWaylandGetKeyboardOcclusion(&top, &overlap);
	return expected_height > 0 ? result && top == expected_height && overlap == 0 :
		!result && top == -7 && overlap == -9;
}

static void request(int visible)
{
	/* A pre-focus request must retain its baseline for the later native enter. */
	fb_wayland_window.text_input_entered = 0;
	require(fb_wayland_set_keyboard_locked(visible) == -1, "pre-focus fallback changed");
	fb_wayland_window.text_input_entered = 1;
}

int main(void)
{
	FB_WAYLAND_API api;
	memset(&api, 0, sizeof(api));
	api.proxy_marshal = acknowledge;
	fb_wayland_window.api = &api;
	fb_wayland_window.active = 1;
	fb_wayland_window.xdg_surface = (struct xdg_surface *)(uintptr_t)1;
	fb_wayland_window.text_input = (struct zwp_text_input_v3 *)(uintptr_t)1;
	context.flags = DRIVER_RESIZABLE;
	configure(360, 648, 1);
	require(marker(0), "initial shell chrome marked as keyboard space");
	request(1);
	fb_wayland_toplevel_configure(NULL, NULL, 360, 605, NULL);
	/* A role callback alone must not change the committed state. */
	require(fb_wayland_window.maximized, "role state committed before surface configure");
	configure(360, 605, 1);
	require(marker(605), "small acknowledged reservation was discarded");
	request(1);
	require(fb_wayland_window.keyboard_base_height == 648 && marker(605),
		"repeated show replaced the unoccluded baseline");
	configure(605, 360, 1);
	require(marker(360), "rotation lost the known reservation");
	configure(360, 648, 1);
	require(marker(0), "restored viewport retained an empty-overlap marker");
	configure(360, 605, 1);
	request(0);
	require(marker(0), "hidden request exposed a viewport marker");
	request(1);
	require(marker(605) && fb_wayland_window.keyboard_base_height == 648,
		"editor transfer replaced the still-reserved baseline");
	request(0);
	configure(360, 648, 1);
	require(!fb_wayland_window.keyboard_viewport_reserved,
		"full native restore retained the reservation");
	request(1);
	configure(360, 605, 0);
	require(marker(0), "floating resize claimed keyboard reservation");
	context.flags |= DRIVER_FULLSCREEN;
	configure(360, 550, 1);
	require(marker(0), "fullscreen resize claimed keyboard reservation");
	context.flags = 0;
	configure(360, 500, 1);
	require(marker(0), "fixed-size surface claimed keyboard reservation");
	printf("wayland-keyboard-reservation: PASS checks=%u\n", checks);
	return EXIT_SUCCESS;
}

/* end of wayland-keyboard-reservation.c */
