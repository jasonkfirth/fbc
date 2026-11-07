/*
 * Project: FreeBASIC gfxlib2 Linux Wayland support
 * File: fb_wayland_client.c
 * Purpose: Load libwayland-client only when a Wayland backend is used.
 * Responsibilities: Own the process-lifetime library handle, ABI table and
 *     selection of a Wayland application identity from launcher metadata.
 * This file intentionally does not create windows or infer handheld status.
 */
#define _POSIX_C_SOURCE 200809L

#include "fb_wayland_client.h"

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static const char *const fb_wayland_function_names[] = {
	"wl_display_connect",
	"wl_display_disconnect",
	"wl_display_roundtrip",
	"wl_proxy_marshal_constructor_versioned",
	"wl_proxy_get_version",
	"wl_proxy_add_listener",
	"wl_proxy_destroy",
	NULL
};

static FB_WAYLAND_API fb_wayland_api;
static FB_DYLIB fb_wayland_library;
static pthread_mutex_t fb_wayland_loader_mutex = PTHREAD_MUTEX_INITIALIZER;
static int fb_wayland_loader_attempted;
static int fb_wayland_loader_result = -1;

int fb_hWaylandShouldUse(void)
{
	const char *mir_socket = getenv("MIR_SOCKET");
	const char *x11_display = getenv("DISPLAY");
	const char *desktop_hint = getenv("DESKTOP_FILE_HINT");

	/*
		Lomiri admits a native Wayland client when its launcher supplies a
		desktop-file identity. An unregistered shell may have both display
		variables but cannot use that path, so retain its Xwayland fallback.
	*/
	if ((mir_socket != NULL) && (mir_socket[0] != '\0') &&
		(x11_display != NULL) && (x11_display[0] != '\0') &&
		((desktop_hint == NULL) || (desktop_hint[0] == '\0')))
		return 0;
	return 1;
}

/*
 * xdg_toplevel identifies an application by its desktop-file basename.
 * Lomiri supplies that identity in DESKTOP_FILE_HINT; APP_ID is a fallback
 * for launchers that only supply their application identifier. Reject names
 * outside the desktop-entry character set instead of sending invalid data to
 * the compositor or silently truncating an identity.
 */
static int fb_wayland_copy_app_id(const char *value, char *buffer, size_t capacity)
{
	const char *basename;
	size_t length, i;

	if (!value || !*value || !buffer || capacity == 0)
		return 0;
	basename = strrchr(value, '/');
	basename = basename ? basename + 1 : value;
	length = strlen(basename);
	if (length >= 8 && strcmp(basename + length - 8, ".desktop") == 0)
		length -= 8;
	if (length == 0 || length >= capacity)
		return 0;
	for (i = 0; i < length; i++) {
		unsigned char ch = (unsigned char)basename[i];
		if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
		      (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-'))
			return 0;
	}
	memcpy(buffer, basename, length);
	buffer[length] = '\0';
	return 1;
}

const char *fb_hWaylandAppId(char *buffer, size_t capacity, const char *fallback)
{
	if (fb_wayland_copy_app_id(getenv("DESKTOP_FILE_HINT"), buffer, capacity) ||
	    fb_wayland_copy_app_id(getenv("APP_ID"), buffer, capacity))
		return buffer;
	return fallback;
}

extern void fb_hWaylandInitXdgProtocol(const struct wl_interface *output,
	const struct wl_interface *seat, const struct wl_interface *surface);
extern void fb_hWaylandInitTextInputProtocol(const struct wl_interface *seat,
	const struct wl_interface *surface);
extern void fb_hWaylandInitTextInputV2Protocol(const struct wl_interface *seat,
	const struct wl_interface *surface);

static void fb_wayland_load_optional(const char *name, void **address)
{
	const char *const names[] = { name };
	void *symbol = NULL;

	if (fb_hDynLoadAlso(fb_wayland_library, names, &symbol, 1) == 0)
		*address = symbol;
}

int fb_hWaylandGetApi(const FB_WAYLAND_API **api)
{
	void *symbols[7];
	void *optional_symbols[7] = { NULL };

	if (api == NULL)
		return -1;

	pthread_mutex_lock(&fb_wayland_loader_mutex);
	if (!fb_wayland_loader_attempted) {
		fb_wayland_loader_attempted = 1;
		fb_wayland_library = fb_hDynLoad("libwayland-client.so.0",
			fb_wayland_function_names, symbols);
		if (fb_wayland_library) {
			fb_wayland_api.display_connect =
				(struct wl_display *(*)(const char *))symbols[0];
			fb_wayland_api.display_disconnect =
				(void (*)(struct wl_display *))symbols[1];
			fb_wayland_api.display_roundtrip =
				(int (*)(struct wl_display *))symbols[2];
			fb_wayland_load_optional("wl_display_dispatch_pending",
				&optional_symbols[0]);
			fb_wayland_load_optional("wl_display_get_fd",
				&optional_symbols[1]);
			fb_wayland_load_optional("wl_display_flush",
				&optional_symbols[2]);
			fb_wayland_load_optional("wl_display_prepare_read",
				&optional_symbols[3]);
			fb_wayland_load_optional("wl_display_read_events",
				&optional_symbols[4]);
			fb_wayland_load_optional("wl_display_cancel_read",
				&optional_symbols[5]);
			fb_wayland_load_optional("wl_proxy_marshal",
				&optional_symbols[6]);
			fb_wayland_api.display_dispatch_pending =
				(int (*)(struct wl_display *))optional_symbols[0];
			fb_wayland_api.display_get_fd =
				(int (*)(struct wl_display *))optional_symbols[1];
			fb_wayland_api.display_flush =
				(int (*)(struct wl_display *))optional_symbols[2];
			fb_wayland_api.display_prepare_read =
				(int (*)(struct wl_display *))optional_symbols[3];
			fb_wayland_api.display_read_events =
				(int (*)(struct wl_display *))optional_symbols[4];
			fb_wayland_api.display_cancel_read =
				(void (*)(struct wl_display *))optional_symbols[5];
			fb_wayland_api.proxy_marshal =
				(void (*)(struct wl_proxy *, uint32_t, ...))optional_symbols[6];
			fb_wayland_api.proxy_marshal_constructor_versioned =
				(struct wl_proxy *(*)(struct wl_proxy *, uint32_t,
					const struct wl_interface *, uint32_t, ...))symbols[3];
			fb_wayland_api.proxy_get_version =
				(uint32_t (*)(struct wl_proxy *))symbols[4];
			fb_wayland_api.proxy_add_listener =
				(int (*)(struct wl_proxy *, void (**)(void), void *))symbols[5];
			fb_wayland_api.proxy_destroy =
				(void (*)(struct wl_proxy *))symbols[6];
			fb_wayland_loader_result = 0;
		}
	}
	if (fb_wayland_loader_result == 0)
		*api = &fb_wayland_api;
	pthread_mutex_unlock(&fb_wayland_loader_mutex);
	return fb_wayland_loader_result;
}

int fb_hWaylandLoadSymbols(const char *const *names, void **addresses,
	size_t count)
{
	const FB_WAYLAND_API *api;

	if ((names == NULL) || (addresses == NULL) || (count == 0) ||
		(fb_hWaylandGetApi(&api) != 0) || (fb_wayland_library == NULL))
		return -1;
	(void)api;
	if (count > (size_t)SSIZE_MAX)
		return -1;
	return fb_hDynLoadAlso(fb_wayland_library, names, addresses,
		(ssize_t)count);
}

int fb_hWaylandInitProtocols(void)
{
	static const char *const interface_names[] = {
		"wl_output_interface", "wl_seat_interface", "wl_surface_interface"
	};
	void *interfaces[3];

	if ((fb_hWaylandLoadSymbols(interface_names, interfaces, 3) != 0) ||
		(interfaces[0] == NULL) || (interfaces[1] == NULL) ||
		(interfaces[2] == NULL))
		return -1;

	fb_hWaylandInitXdgProtocol(
		(const struct wl_interface *)interfaces[0],
		(const struct wl_interface *)interfaces[1],
		(const struct wl_interface *)interfaces[2]);
	fb_hWaylandInitTextInputProtocol(
		(const struct wl_interface *)interfaces[1],
		(const struct wl_interface *)interfaces[2]);
	fb_hWaylandInitTextInputV2Protocol(
		(const struct wl_interface *)interfaces[1],
		(const struct wl_interface *)interfaces[2]);
	return 0;
}

/* end of fb_wayland_client.c */
