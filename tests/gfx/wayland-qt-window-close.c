/*
 * Project: FreeBASIC gfxlib2 Linux tests
 * File: wayland-qt-window-close.c
 * Purpose: Check Qt mobile window-manager quit delivery and proxy ownership.
 * Responsibilities: Cover the published version-one ABI, optional binding,
 *     failed listener installation, stale events and ordinary close events.
 * This file does not connect to a compositor or terminate an application.
 */
#include "../../src/gfxlib2/drivers/linux/gfx_driver_wayland.c"
#include <stdarg.h>

static unsigned checks, binds, listeners, destroys, close_events;
static int fail_bind, fail_listener;
static FB_WAYLAND_WINDOW *listener_data;
static const FB_WL_QT_WINDOWMANAGER_LISTENER *installed_listener;

static void require(int condition, const char *message)
{
	checks++;
	if (!condition) {
		fprintf(stderr, "wayland-qt-window-close: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

void fb_hPostEvent(EVENT *event)
{
	require(event->type == EVENT_WINDOW_CLOSE, "quit changed the event kind");
	close_events++;
}

static struct wl_proxy *record_bind(struct wl_proxy *registry, uint32_t opcode,
	const struct wl_interface *interface, uint32_t version, ...)
{
	(void)registry;
	binds++;
	require(opcode == FB_WL_REGISTRY_BIND && version == 1,
		"optional manager was not bound at the supported version");
	require(strcmp(interface->name, "qt_windowmanager") == 0 && interface->version == 1,
		"optional interface identity changed");
	return fail_bind ? NULL : (struct wl_proxy *)(uintptr_t)1;
}

static int record_listener(struct wl_proxy *proxy, void (**implementation)(void), void *data)
{
	(void)proxy;
	listeners++;
	installed_listener = (const FB_WL_QT_WINDOWMANAGER_LISTENER *)implementation;
	listener_data = (FB_WAYLAND_WINDOW *)data;
	return fail_listener ? -1 : 0;
}

static void record_destroy(struct wl_proxy *proxy)
{
	require(proxy == (struct wl_proxy *)(uintptr_t)1, "destroyed an unowned proxy");
	destroys++;
}

int main(void)
{
	FB_WAYLAND_API api;
	FB_WAYLAND_WINDOW *window = &fb_wayland_window;
	struct wl_registry *registry = (struct wl_registry *)(uintptr_t)2;
	struct qt_windowmanager *manager = (struct qt_windowmanager *)(uintptr_t)1;
	const struct wl_interface *interface = &fb_wayland_qt_windowmanager_interface;
	memset(&api, 0, sizeof(api));
	memset(window, 0, sizeof(*window));
	api.proxy_marshal_constructor_versioned = record_bind;
	api.proxy_add_listener = record_listener;
	api.proxy_destroy = record_destroy;
	window->api = &api;
	require(interface->method_count == 1 && interface->event_count == 2,
		"version-one message counts changed");
	require(strcmp(interface->methods[0].name, "open_url") == 0 &&
		strcmp(interface->methods[0].signature, "us") == 0 &&
		strcmp(interface->events[0].name, "hints") == 0 &&
		strcmp(interface->events[0].signature, "i") == 0 &&
		strcmp(interface->events[1].name, "quit") == 0 &&
		strcmp(interface->events[1].signature, "") == 0, "published wire ABI changed");
	fb_wayland_qt_windowmanager_bind(window, registry, 7, 0);
	require(binds == 0, "unsupported zero version was bound");
	fail_bind = 1;
	fb_wayland_qt_windowmanager_bind(window, registry, 7, 1);
	require(!window->qt_windowmanager && listeners == 0, "failed bind installed a listener");
	fail_bind = 0;
	fail_listener = 1;
	fb_wayland_qt_windowmanager_bind(window, registry, 7, 1);
	require(!window->qt_windowmanager && destroys == 1, "failed listener retained a proxy");
	fail_listener = 0;
	fb_wayland_qt_windowmanager_bind(window, registry, 7, 9);
	require(window->qt_windowmanager == manager && listener_data == window,
		"manager listener lost its mode context");
	fb_wayland_qt_windowmanager_bind(window, registry, 8, 1);
	require(binds == 3, "duplicate manager was rebound");
	installed_listener->quit(listener_data, manager);
	require(close_events == 0, "inactive mode received a close");
	window->active = 1;
	installed_listener->hints(listener_data, manager, 1);
	require(close_events == 0, "window-state hint closed the application");
	installed_listener->quit(NULL, manager);
	installed_listener->quit(listener_data, NULL);
	installed_listener->quit(listener_data, (struct qt_windowmanager *)(uintptr_t)3);
	require(close_events == 0, "stale or missing mode received a close");
	installed_listener->quit(listener_data, manager);
	require(close_events == 1, "client quit did not deliver the ordinary close event");
	fb_wayland_qt_windowmanager_destroy(window);
	fb_wayland_qt_windowmanager_destroy(window);
	require(destroys == 2 && !window->qt_windowmanager, "manager cleanup was not idempotent");
	installed_listener->quit(listener_data, manager);
	require(close_events == 1, "released manager delivered a late close");
	fb_wayland_toplevel_close(window, NULL);
	require(close_events == 2, "ordinary xdg close behavior changed");
	printf("wayland-qt-window-close: PASS checks=%u\n", checks);
	return EXIT_SUCCESS;
}

/* end of wayland-qt-window-close.c */
