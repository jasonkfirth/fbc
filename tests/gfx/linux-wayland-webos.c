/*
 * Project: FreeBASIC Linux graphics tests
 * File: linux-wayland-webos.c
 * Purpose: Check legacy webOS protocol layouts and text-focus lifetime.
 * Responsibilities: Exercise release-specific event orders, reject unsafe
 *     signatures, preserve focus lifetime and check acknowledged panel refresh.
 * This file does not require a compositor or replace native application tests.
 */
#include <assert.h>
#include <stdio.h>
#include "../../src/gfxlib2/drivers/linux/fb_wayland_webos.c"

static int tokens[8];
#define PROXY(i) ((struct wl_proxy *)&tokens[i])
static int messages, commits, keys, deletes, sizes, closes;
static uint32_t last_opcode;
static int model_creates, last_width, last_height;

FB_DYLIB fb_hDynLoad(const char *name, const char *const *names, void **symbols)
{
	(void)name; (void)names; (void)symbols;
	return NULL;
}

static const struct wl_message model_requests[] = {
	{ "activate", "uoo", NULL }, { "deactivate", "o", NULL },
	{ "set_cursor_rectangle", "iiii", NULL }, { "set_content_type", "uu", NULL },
	{ "commit", "", NULL }, { "show_input_panel", "", NULL },
	{ "hide_input_panel", "", NULL }
};
static const struct wl_message model_events[] = {
	{ "leave", "", NULL }, { "commit_string", "us", NULL },
	{ "enter", "o", NULL }, { "input_panel_state", "u", NULL }
};
static const struct wl_message shell_events_luneos[] = {
	{ "state_changed", "u", NULL }, { "position_changed", "ii", NULL },
	{ "client_size_changed", "ii", NULL }, { "close", "", NULL },
	{ "exposed", "a", NULL }, { "state_about_to_change", "u", NULL },
	{ "addon_status_changed", "2u", NULL }
};
static const struct wl_message shell_events_ose[] = {
	{ "state_changed", "u", NULL }, { "position_changed", "ii", NULL },
	{ "close", "", NULL }, { "exposed", "a", NULL },
	{ "state_about_to_change", "u", NULL }, { "addon_status_changed", "2u", NULL }
};
static const struct wl_message factory_requests[] = { { "create_text_model", "n", NULL } };
static const struct wl_message shell_requests[] = { { "get_shell_surface", "no", NULL } };
static const struct wl_message surface_requests[] = { { "set_property", "ss", NULL } };
static struct wl_interface model = { "text_model", 1, 7, model_requests, 4, model_events };
static struct wl_interface factory = { "text_model_factory", 1, 1, factory_requests, 0, NULL };
static struct wl_interface shell = { "wl_webos_shell", 2, 1, shell_requests, 0, NULL };
static struct wl_interface surface = { "wl_webos_shell_surface", 2, 1, surface_requests, 7, shell_events_luneos };

static struct wl_proxy *construct(struct wl_proxy *proxy, uint32_t opcode,
	const struct wl_interface *interface, uint32_t version, ...)
{
	(void)proxy; (void)opcode;
	assert(version == 1);
	if (interface == &model)
		return ++model_creates == 1 ? PROXY(2) : PROXY(6);
	return PROXY(3);
}
static int listen(struct wl_proxy *proxy, void (**callbacks)(void), void *data)
{
	(void)proxy; (void)callbacks; (void)data;
	return 0;
}
static void marshal(struct wl_proxy *proxy, uint32_t opcode, ...)
{
	(void)proxy;
	messages++;
	last_opcode = opcode;
}
static void destroy(struct wl_proxy *proxy) { (void)proxy; }
static void text(void *data, const char *value)
{
	(void)data;
	assert(strcmp(value, "q") == 0);
	commits++;
}
static void key(void *data, uint32_t symbol, uint32_t state)
{
	(void)data; (void)state;
	assert(symbol == 0xff08);
	keys++;
}
static void delete_before(void *data, uint32_t count)
{
	(void)data;
	assert(count == 1);
	deletes++;
}
static void resize(void *data, int width, int height)
{
	(void)data;
	assert(width > 0 && height > 0);
	last_width = width;
	last_height = height;
	sizes++;
}
static void close_window(void *data) { (void)data; closes++; }

int main(void)
{
	FB_WAYLAND_WEBOS webos = { 0 };
	FB_WAYLAND_API api = { 0 };
	FB_WAYLAND_WEBOS_CALLBACKS callbacks = { text, key, delete_before, resize, close_window };
	struct wl_message invalid = { "close", "i", NULL };
	int previous;
	api.proxy_marshal_constructor_versioned = construct;
	api.proxy_add_listener = listen;
	api.proxy_marshal = marshal;
	api.proxy_destroy = destroy;
	webos.api = &api;
	webos.factory = PROXY(0);
	webos.shell = PROXY(1);
	webos_factory_interface = &factory;
	webos_model_interface = &model;
	webos_shell_interface = &shell;
	webos_surface_interface = &surface;
	fb_hWaylandWebosOpen(&webos, PROXY(4), PROXY(5), "net.fbxl.templari", &callbacks, NULL, 1);
	assert(webos.model == PROXY(2) && webos.shell_surface == PROXY(3));
	assert(webos.shell_listener[2] == (void (*)(void))webos_size);
	assert(webos.shell_listener[3] == (void (*)(void))webos_close);
	surface.events = shell_events_ose; surface.event_count = 6;
	assert(webos_listeners(&surface, webos.shell_listener, webos_shell_events, 7) == 0);
	assert(webos.shell_listener[2] == (void (*)(void))webos_close);
	surface.events = &invalid; surface.event_count = 1;
	assert(webos_listeners(&surface, webos.shell_listener, webos_shell_events, 7) < 0);
	surface.event_count = FB_WAYLAND_WEBOS_MAX_EVENTS + 1;
	assert(webos_listeners(&surface, webos.shell_listener, webos_shell_events, 7) < 0);
	webos_commit(&webos, webos.model, 1, "q"); assert(commits == 0);
	assert(fb_hWaylandWebosKeyboard(&webos, 1) == 0);
	assert(last_opcode == 5 && webos.visible == -2);
	webos_enter(&webos, webos.model, PROXY(0)); assert(!webos.entered);
	webos_enter(&webos, webos.model, PROXY(5));
	webos_preedit(&webos, webos.model, 1, "q", "q"); assert(commits == 0);
	webos_commit(&webos, webos.model, 1, "q"); assert(commits == 1);
	webos_keysym(&webos, webos.model, 1, 0, 0xff08, 1, 0); assert(keys == 1);
	webos_delete(&webos, webos.model, 1, -1, 1); assert(deletes == 1);
	webos_delete(&webos, webos.model, 1, 0, 1);
	webos_delete(&webos, webos.model, 1, -1, 2); assert(deletes == 1);
	assert(fb_hWaylandWebosRegion(&webos, 10, 20, 0, 30) < 0);
	assert(fb_hWaylandWebosRegion(&webos, 10, 20, 40, 30) == 0);
	previous = messages;
	assert(fb_hWaylandWebosRegion(&webos, 10, 20, 40, 30) == 0);
	assert(messages == previous);
	webos_size(&webos, webos.shell_surface, -1, 480); assert(sizes == 0);
	webos_size(&webos, webos.shell_surface, 640, 480); assert(sizes == 1);
	assert(last_width == 640 && last_height == 480);
	webos_close(&webos, webos.shell_surface); assert(closes == 1);
	webos_panel_state(&webos, webos.model, 1); assert(webos.visible == -1);
	webos_panel_rect(&webos, webos.model, 0, 696, 1920, 354);
	assert(webos.panel_top == 696 && webos.panel_height == 354);
	webos_panel_rect(&webos, webos.model, 0, 696, UINT32_MAX, 354);
	assert(webos.panel_height == 0);
	webos_panel_rect(&webos, webos.model, 0, 696, 1920, 354);
	previous = messages;
	webos_size(&webos, webos.shell_surface, 640, 480);
	assert(messages == previous && !webos.panel_refresh_pending);
	webos_size(&webos, webos.shell_surface, 480, 640);
	assert(messages == previous + 1 && last_opcode == 6);
	assert(webos.panel_refresh_pending && webos.panel_height == 0);
	assert(webos.model == PROXY(2) && model_creates == 1 && webos.entered);
	webos_panel_rect(&webos, webos.model, 0, 696, 1920, 354);
	assert(webos.panel_height == 0);
	webos_size(&webos, webos.shell_surface, 480, 800);
	assert(last_width == 480 && last_height == 800);
	assert(fb_hWaylandWebosKeyboard(&webos, 1) == 0);
	assert(messages == previous + 1);
	webos_panel_state(&webos, webos.model, 0);
	assert(messages == previous + 2 && last_opcode == 5);
	assert(!webos.panel_refresh_pending && webos.visible == -2);
	assert(webos.model == PROXY(2) && model_creates == 1);
	webos_panel_state(&webos, webos.model, 1);
	webos_panel_rect(&webos, webos.model, 0, 446, 480, 354);
	assert(webos.visible == -1 && webos.panel_top == 446 && webos.panel_height == 354);
	webos_size(&webos, webos.shell_surface, 640, 480);
	assert(webos.panel_refresh_pending);
	previous = messages;
	webos_leave(&webos, webos.model);
	assert(messages == previous && webos.model == NULL);
	assert(webos.panel_height == 0 && !webos.panel_refresh_pending);
	webos_panel_state(&webos, PROXY(2), 0);
	assert(messages == previous);
	webos_commit(&webos, webos.model, 1, "q"); assert(commits == 1);
	assert(fb_hWaylandWebosKeyboard(&webos, 0) == 0 && !webos.requested);
	assert(webos.model == NULL && webos.text_ready);
	assert(fb_hWaylandWebosKeyboard(&webos, 1) == 0);
	assert(webos.model == PROXY(6) && model_creates == 2);
	webos_enter(&webos, webos.model, PROXY(5));
	webos_commit(&webos, PROXY(2), 1, "q"); assert(commits == 1);
	webos_leave(&webos, PROXY(2)); assert(webos.entered);
	webos_commit(&webos, webos.model, 2, "q"); assert(commits == 2);
	webos_panel_state(&webos, webos.model, 1);
	webos_size(&webos, webos.shell_surface, 480, 640);
	assert(webos.panel_refresh_pending);
	assert(fb_hWaylandWebosKeyboard(&webos, 0) == 0);
	assert(!webos.panel_refresh_pending && !webos.model);
	previous = messages;
	webos_panel_state(&webos, PROXY(6), 0);
	assert(messages == previous);
	fb_hWaylandWebosDestroy(&webos);
	assert(!webos.api && !webos.model && !webos.shell_surface);
	puts("linux-wayland-webos: PASS");
	return 0;
}

/* end of linux-wayland-webos.c */
