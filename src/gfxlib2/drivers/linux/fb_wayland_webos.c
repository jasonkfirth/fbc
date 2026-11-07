/*
 * Project: FreeBASIC Linux Wayland support
 * File: fb_wayland_webos.c
 * Purpose: Use the native shell and text-model interfaces on LuneOS/webOS.
 * Responsibilities: Bind optional protocols, preserve surface identity and
 *     forward committed IME input through the owning graphics backend.
 * This file does not create wl_surface objects, render, or edit widget text.
 *
 * LuneOS retains the pre-standard text_model protocol. Its installed
 * libwayland-webos-client supplies the matching wire descriptions. Shell
 * event layouts differ even between releases advertising the same version:
 * LuneOS adds client_size_changed where other webOS releases send close.
 * Resolve requests and listeners by both name and signature, not a guessed
 * opcode. Reject unknown events instead of installing a mistyped callback.
 *
 * Protocol reference: webosose/webos-wayland-extensions, protocol/text.xml
 * and protocol/webos-shell.xml. The library and immutable interface pointers
 * live until process exit. The loader mutex protects their one-time setup;
 * each driver serializes its own proxies and callbacks with its window lock.
 */
#include "fb_wayland_webos.h"

#include <pthread.h>
#include <limits.h>
#include <string.h>

struct wl_array;

static pthread_mutex_t webos_loader_mutex = PTHREAD_MUTEX_INITIALIZER;
static int webos_loader_attempted;
static FB_DYLIB webos_library;
static const struct wl_interface *webos_factory_interface;
static const struct wl_interface *webos_model_interface;
static const struct wl_interface *webos_shell_interface;
static const struct wl_interface *webos_surface_interface;

typedef struct FB_WEBOS_EVENT {
	const char *name;
	const char *signature;
	void (*callback)(void);
} FB_WEBOS_EVENT;

/* Wayland prefixes a signature with the version that introduced a message. */
static const char *webos_signature(const char *signature)
{
	if (!signature)
		return "?";
	while (*signature >= '0' && *signature <= '9')
		signature++;
	return signature;
}

static int webos_request(const struct wl_interface *interface,
	const char *name, const char *signature)
{
	int i;
	if (!interface || interface->method_count < 0 ||
		interface->method_count > 32 || !interface->methods)
		return -1;
	for (i = 0; i < interface->method_count; i++) {
		const struct wl_message *message = &interface->methods[i];
		if (message->name && strcmp(message->name, name) == 0 &&
			strcmp(webos_signature(message->signature), signature) == 0)
			return i;
	}
	return -1;
}

static void webos_load(void)
{
	static const char *const names[] = {
		"text_model_factory_interface", "text_model_interface",
		"wl_webos_shell_interface", "wl_webos_shell_surface_interface", NULL
	};
	void *symbols[4];
	pthread_mutex_lock(&webos_loader_mutex);
	if (!webos_loader_attempted) {
		webos_loader_attempted = 1;
		webos_library = fb_hDynLoad("libwayland-webos-client.so.1", names, symbols);
		if (webos_library) {
			webos_factory_interface = symbols[0];
			webos_model_interface = symbols[1];
			webos_shell_interface = symbols[2];
			webos_surface_interface = symbols[3];
		}
	}
	pthread_mutex_unlock(&webos_loader_mutex);
}

static int webos_listeners(const struct wl_interface *interface,
	void (**listeners)(void), const FB_WEBOS_EVENT *known, size_t count)
{
	int i;
	size_t j;
	if (!interface || interface->event_count < 0 ||
		interface->event_count > FB_WAYLAND_WEBOS_MAX_EVENTS ||
		(interface->event_count && !interface->events))
		return -1;
	for (i = 0; i < interface->event_count; i++) {
		const struct wl_message *message = &interface->events[i];
		for (j = 0; j < count; j++) {
			if (message->name && strcmp(message->name, known[j].name) == 0 &&
				strcmp(webos_signature(message->signature), known[j].signature) == 0)
				break;
		}
		if (j == count)
			return -1;
		listeners[i] = known[j].callback;
	}
	return 0;
}

/* ------------------------------------------------------------------------- */
/* Text-model callbacks                                                      */
/* ------------------------------------------------------------------------- */

static void webos_commit(void *data, struct wl_proxy *proxy,
	uint32_t serial, const char *text)
{
	FB_WAYLAND_WEBOS *webos = data;
	(void)proxy; (void)serial;
	if (proxy == webos->model && webos->entered && webos->requested && webos->callbacks.text)
		webos->callbacks.text(webos->user, text);
}

static void webos_preedit(void *data, struct wl_proxy *proxy,
	uint32_t serial, const char *text, const char *commit)
{
	/* Preedit fallback text is not an additional committed string. */
	(void)data; (void)proxy; (void)serial; (void)text; (void)commit;
}

static void webos_delete(void *data, struct wl_proxy *proxy,
	uint32_t serial, int32_t index, uint32_t length)
{
	FB_WAYLAND_WEBOS *webos = data;
	(void)proxy; (void)serial;
	/* The legacy graphics queue supports backspace, not arbitrary text ranges. */
	if (proxy == webos->model && webos->entered && webos->requested && index < 0 &&
		(uint64_t)length <= (uint64_t)(-(int64_t)index) &&
		webos->callbacks.delete_before)
		webos->callbacks.delete_before(webos->user, length);
}

static void webos_cursor(void *data, struct wl_proxy *proxy,
	uint32_t serial, int32_t index, int32_t anchor)
{
	(void)data; (void)proxy; (void)serial; (void)index; (void)anchor;
}

static void webos_styling(void *data, struct wl_proxy *proxy,
	uint32_t serial, uint32_t index, uint32_t length, uint32_t style)
{
	(void)data; (void)proxy; (void)serial; (void)index; (void)length; (void)style;
}

static void webos_preedit_cursor(void *data, struct wl_proxy *proxy,
	uint32_t serial, int32_t index)
{
	(void)data; (void)proxy; (void)serial; (void)index;
}

static void webos_array(void *data, struct wl_proxy *proxy, struct wl_array *array)
{
	(void)data; (void)proxy; (void)array;
}

static void webos_keysym(void *data, struct wl_proxy *proxy,
	uint32_t serial, uint32_t time, uint32_t symbol, uint32_t state,
	uint32_t modifiers)
{
	FB_WAYLAND_WEBOS *webos = data;
	(void)proxy; (void)serial; (void)time; (void)modifiers;
	if (proxy == webos->model && webos->entered && webos->requested && webos->callbacks.key)
		webos->callbacks.key(webos->user, symbol, state);
}

static void webos_enter(void *data, struct wl_proxy *proxy, struct wl_proxy *surface)
{
	FB_WAYLAND_WEBOS *webos = data;
	if (proxy != webos->model)
		return;
	webos->entered = surface == webos->surface;
}

static void webos_leave(void *data, struct wl_proxy *proxy)
{
	FB_WAYLAND_WEBOS *webos = data;
	if (proxy != webos->model)
		return;
	fb_hWaylandWebosBlur(webos);
}

static void webos_panel_state(void *data, struct wl_proxy *proxy, uint32_t state)
{
	FB_WAYLAND_WEBOS *webos = data;
	if (proxy != webos->model)
		return;
	webos->visible = state ? -1 : 0;
	if (!state)
		webos->panel_height = 0;
	if (!state && webos->panel_refresh_pending) {
		webos->panel_refresh_pending = 0;
		if (webos->requested && webos->entered) {
			webos->api->proxy_marshal(webos->model, (uint32_t)webos_request(
				webos_model_interface, "show_input_panel", ""));
			webos->visible = -2;
		}
	}
}

static void webos_panel_rect(void *data, struct wl_proxy *proxy,
	int32_t x, int32_t y, uint32_t width, uint32_t height)
{
	FB_WAYLAND_WEBOS *webos = data;
	(void)x;
	if (proxy != webos->model || webos->panel_refresh_pending)
		return;
	/* The panel overlays the drawable; its rect is in surface coordinates. */
	if (width > 0 && width <= INT_MAX && height > 0 && height <= INT_MAX) {
		webos->panel_top = y;
		webos->panel_height = (int)height;
	} else {
		webos->panel_height = 0;
	}
}

#define WEBOS_EVENT(name, signature, callback) \
	{ name, signature, (void (*)(void))callback }

static const FB_WEBOS_EVENT webos_text_events[] = {
	WEBOS_EVENT("commit_string", "us", webos_commit),
	WEBOS_EVENT("preedit_string", "uss", webos_preedit),
	WEBOS_EVENT("delete_surrounding_text", "uiu", webos_delete),
	WEBOS_EVENT("cursor_position", "uii", webos_cursor),
	WEBOS_EVENT("preedit_styling", "uuuu", webos_styling),
	WEBOS_EVENT("preedit_cursor", "ui", webos_preedit_cursor),
	WEBOS_EVENT("modifiers_map", "a", webos_array),
	WEBOS_EVENT("keysym", "uuuuu", webos_keysym),
	WEBOS_EVENT("enter", "o", webos_enter),
	WEBOS_EVENT("leave", "", webos_leave),
	WEBOS_EVENT("input_panel_state", "u", webos_panel_state),
	WEBOS_EVENT("input_panel_rect", "iiuu", webos_panel_rect)
};

/* ------------------------------------------------------------------------- */
/* Shell extension callbacks                                                 */
/* ------------------------------------------------------------------------- */

static void webos_state(void *data, struct wl_proxy *proxy, uint32_t state)
{
	(void)data; (void)proxy; (void)state;
}

static void webos_position(void *data, struct wl_proxy *proxy, int32_t x, int32_t y)
{
	(void)data; (void)proxy; (void)x; (void)y;
}

static void webos_size(void *data, struct wl_proxy *proxy, int32_t width, int32_t height)
{
	FB_WAYLAND_WEBOS *webos = data;
	(void)proxy;
	if (width <= 0 || height <= 0)
		return;
	if (width != webos->surface_width || height != webos->surface_height) {
		webos->surface_width = width;
		webos->surface_height = height;
		/*
		 * LuneOS can retain the old panel geometry across client rotation.
		 * Keep the active model, hide its panel, and wait for the hidden
		 * acknowledgement before showing it against the new geometry.
		 * Deactivate would destroy the server model and race the IME context.
		 */
		if (webos->model && webos->requested && webos->entered &&
			webos->visible == -1 && !webos->panel_refresh_pending) {
			webos->panel_refresh_pending = 1;
			webos->panel_height = 0;
			webos->api->proxy_marshal(webos->model, (uint32_t)webos_request(
				webos_model_interface, "hide_input_panel", ""));
		}
	}
	if (webos->callbacks.resize)
		webos->callbacks.resize(webos->user, width, height);
}

static void webos_close(void *data, struct wl_proxy *proxy)
{
	FB_WAYLAND_WEBOS *webos = data;
	(void)proxy;
	if (webos->callbacks.close)
		webos->callbacks.close(webos->user);
}

static const FB_WEBOS_EVENT webos_shell_events[] = {
	WEBOS_EVENT("state_changed", "u", webos_state),
	WEBOS_EVENT("position_changed", "ii", webos_position),
	WEBOS_EVENT("client_size_changed", "ii", webos_size),
	WEBOS_EVENT("close", "", webos_close),
	WEBOS_EVENT("exposed", "a", webos_array),
	WEBOS_EVENT("state_about_to_change", "u", webos_state),
	WEBOS_EVENT("addon_status_changed", "u", webos_state)
};

void fb_hWaylandWebosGlobal(FB_WAYLAND_WEBOS *webos,
	const FB_WAYLAND_API *api, struct wl_proxy *registry, uint32_t name,
	const char *interface_name, uint32_t version)
{
	const struct wl_interface *interface;
	struct wl_proxy **target;
	if (!webos || !api || !registry || !interface_name || version == 0)
		return;
	if (strcmp(interface_name, "text_model_factory") == 0) {
		webos_load();
		interface = webos_factory_interface;
		target = &webos->factory;
	} else if (strcmp(interface_name, "wl_webos_shell") == 0) {
		webos_load();
		interface = webos_shell_interface;
		target = &webos->shell;
	} else {
		return;
	}
	if (*target || !interface || interface->version < 1)
		return;
	webos->api = api;
	if (version > (uint32_t)interface->version)
		version = (uint32_t)interface->version;
	/* Both extensions work with version one; never request unneeded features. */
	version = 1;
	*target = api->proxy_marshal_constructor_versioned(registry, 0, interface,
		version, name, interface->name, version, NULL);
}

static void webos_destroy_proxy(FB_WAYLAND_WEBOS *webos, struct wl_proxy **proxy)
{
	if (*proxy) {
		webos->api->proxy_destroy(*proxy);
		*proxy = NULL;
	}
}

static int webos_create_model(FB_WAYLAND_WEBOS *webos)
{
	int opcode = webos_request(webos_factory_interface, "create_text_model", "n");
	if (!webos->text_ready || opcode < 0)
		return -1;
	webos->model = webos->api->proxy_marshal_constructor_versioned(webos->factory,
		(uint32_t)opcode, webos_model_interface, 1, NULL);
	if (webos->model && webos->api->proxy_add_listener(webos->model,
		webos->text_listener, webos) != 0)
		webos_destroy_proxy(webos, &webos->model);
	return webos->model ? 0 : -1;
}

void fb_hWaylandWebosOpen(FB_WAYLAND_WEBOS *webos, struct wl_proxy *seat,
	struct wl_proxy *surface, const char *app_id,
	const FB_WAYLAND_WEBOS_CALLBACKS *callbacks, void *user, int text_input)
{
	int opcode, property;
	static const char *const required[] = {
		"activate", "deactivate", "set_cursor_rectangle", "set_content_type",
		"commit", "show_input_panel", "hide_input_panel"
	};
	static const char *const signatures[] = { "uoo", "o", "iiii", "uu", "", "", "" };
	size_t i;
	if (!webos || !webos->api || !surface || !callbacks)
		return;
	webos->seat = seat;
	webos->surface = surface;
	webos->callbacks = *callbacks;
	webos->user = user;
	webos->width = webos->height = 1;
	opcode = webos_request(webos_shell_interface, "get_shell_surface", "no");
	property = webos_request(webos_surface_interface, "set_property", "ss");
	if (webos->shell && app_id && *app_id && opcode >= 0 && property >= 0 &&
		webos_listeners(webos_surface_interface, webos->shell_listener,
			webos_shell_events, sizeof(webos_shell_events) / sizeof(webos_shell_events[0])) == 0) {
		webos->shell_surface = webos->api->proxy_marshal_constructor_versioned(
			webos->shell, (uint32_t)opcode, webos_surface_interface, 1, NULL, surface);
		if (webos->shell_surface && webos->api->proxy_add_listener(
			webos->shell_surface, webos->shell_listener, webos) == 0)
			webos->api->proxy_marshal(webos->shell_surface, (uint32_t)property,
				"appId", app_id);
		else
			webos_destroy_proxy(webos, &webos->shell_surface);
	}
	if (!text_input || !seat || !webos->factory)
		return;
	for (i = 0; i < sizeof(required) / sizeof(required[0]); i++)
		if (webos_request(webos_model_interface, required[i], signatures[i]) < 0)
			return;
	opcode = webos_request(webos_factory_interface, "create_text_model", "n");
	if (opcode < 0 || webos_listeners(webos_model_interface, webos->text_listener,
		webos_text_events, sizeof(webos_text_events) / sizeof(webos_text_events[0])) != 0)
		return;
	webos->text_ready = 1;
	(void)webos_create_model(webos);
}

int fb_hWaylandWebosKeyboard(FB_WAYLAND_WEBOS *webos, int visible)
{
	if (!webos || !webos->text_ready)
		return -1;
	visible = visible != 0;
	if (!visible)
		webos->panel_refresh_pending = 0;
	else if (webos->panel_refresh_pending)
		return 0;
	if (!visible && !webos->requested)
		return 0;
	if (visible && !webos->model && webos_create_model(webos) != 0)
		return -1;
	if (visible && (!webos->requested || !webos->entered)) {
		webos->serial++;
		webos->api->proxy_marshal(webos->model,
			(uint32_t)webos_request(webos_model_interface, "activate", "uoo"),
			webos->serial, webos->seat, webos->surface);
		webos->api->proxy_marshal(webos->model,
			(uint32_t)webos_request(webos_model_interface, "set_content_type", "uu"), 0u, 0u);
		webos->api->proxy_marshal(webos->model,
			(uint32_t)webos_request(webos_model_interface, "set_cursor_rectangle", "iiii"),
			webos->x, webos->y, webos->width, webos->height);
		webos->api->proxy_marshal(webos->model,
			(uint32_t)webos_request(webos_model_interface, "commit", ""));
		webos->visible = -2;
	}
	webos->requested = visible;
	webos->api->proxy_marshal(webos->model, (uint32_t)webos_request(
		webos_model_interface, visible ? "show_input_panel" : "hide_input_panel", ""));
	if (!visible) {
		webos->api->proxy_marshal(webos->model,
			(uint32_t)webos_request(webos_model_interface, "deactivate", "o"), webos->seat);
		webos->entered = 0;
		webos->visible = 0;
		webos->panel_height = 0;
		/*
		 * LuneOS deletes this server object on deactivate (delete_id),
		 * even though the protocol has no explicit destroy request.
		 * Release our proxy too; the next activation creates a fresh model.
		 */
		webos_destroy_proxy(webos, &webos->model);
	}
	return 0;
}

void fb_hWaylandWebosBlur(FB_WAYLAND_WEBOS *webos)
{
	if (!webos || !webos->api)
		return;
	/* The compositor already deactivated this surface. Do not send a second
	 * deactivate to an object that LuneOS may already have deleted. */
	webos_destroy_proxy(webos, &webos->model);
	webos->requested = 0;
	webos->entered = 0;
	webos->visible = 0;
	webos->panel_height = 0;
	webos->panel_refresh_pending = 0;
}

int fb_hWaylandWebosRegion(FB_WAYLAND_WEBOS *webos,
	int x, int y, int width, int height)
{
	if (!webos || !webos->text_ready || width <= 0 || height <= 0)
		return -1;
	if (webos->x == x && webos->y == y && webos->width == width && webos->height == height)
		return 0;
	webos->x = x; webos->y = y; webos->width = width; webos->height = height;
	if (webos->requested && webos->model) {
		webos->api->proxy_marshal(webos->model,
			(uint32_t)webos_request(webos_model_interface, "set_cursor_rectangle", "iiii"),
			x, y, width, height);
		webos->api->proxy_marshal(webos->model,
			(uint32_t)webos_request(webos_model_interface, "commit", ""));
	}
	return 0;
}

void fb_hWaylandWebosDestroy(FB_WAYLAND_WEBOS *webos)
{
	if (!webos || !webos->api)
		return;
	if (webos->model && webos->requested)
		(void)fb_hWaylandWebosKeyboard(webos, 0);
	/* These legacy interfaces have no wire destroy request. */
	webos_destroy_proxy(webos, &webos->model);
	webos_destroy_proxy(webos, &webos->factory);
	webos_destroy_proxy(webos, &webos->shell_surface);
	webos_destroy_proxy(webos, &webos->shell);
	memset(webos, 0, sizeof(*webos));
}

/* end of fb_wayland_webos.c */
