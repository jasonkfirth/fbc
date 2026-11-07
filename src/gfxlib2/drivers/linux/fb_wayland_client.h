/*
 * Project: FreeBASIC gfxlib2 Linux Wayland support
 * File: fb_wayland_client.h
 * Purpose: Share the optional Wayland client ABI across Linux backends.
 * Responsibilities: Define the protocol metadata, loaded entry points and
 *     shared desktop application identity.
 * This file intentionally does not create windows or choose input policy.
 */
#ifndef FB_WAYLAND_CLIENT_H
#define FB_WAYLAND_CLIENT_H

#include <sys/types.h>

#include "../../../rtlib/fb_config.h"
#include "../../../rtlib/fb_private_hdynload.h"
#include "protocols/wayland-util.h"

#include <stddef.h>
#include <stdint.h>

struct wl_display;
struct wl_proxy;

/* D-Bus well-known names, which desktop files should use, are at most 255 bytes. */
#define FB_WAYLAND_APP_ID_CAPACITY 256

typedef struct FB_WAYLAND_API {
	struct wl_display *(*display_connect)(const char *name);
	void (*display_disconnect)(struct wl_display *display);
	int (*display_roundtrip)(struct wl_display *display);
	int (*display_dispatch_pending)(struct wl_display *display);
	int (*display_get_fd)(struct wl_display *display);
	int (*display_flush)(struct wl_display *display);
	int (*display_prepare_read)(struct wl_display *display);
	int (*display_read_events)(struct wl_display *display);
	void (*display_cancel_read)(struct wl_display *display);
	void (*proxy_marshal)(struct wl_proxy *proxy, uint32_t opcode, ...);
	struct wl_proxy *(*proxy_marshal_constructor_versioned)(
		struct wl_proxy *proxy, uint32_t opcode,
		const struct wl_interface *interface, uint32_t version, ...);
	uint32_t (*proxy_get_version)(struct wl_proxy *proxy);
	int (*proxy_add_listener)(struct wl_proxy *proxy,
		void (**implementation)(void), void *data);
	void (*proxy_destroy)(struct wl_proxy *proxy);
} FB_WAYLAND_API;

int fb_hWaylandGetApi(const FB_WAYLAND_API **api);
int fb_hWaylandLoadSymbols(const char *const *names, void **addresses,
	size_t count);
int fb_hWaylandInitProtocols(void);
int fb_hWaylandShouldUse(void);
const char *fb_hWaylandAppId(char *buffer, size_t capacity, const char *fallback);

#endif

/* end of fb_wayland_client.h */
