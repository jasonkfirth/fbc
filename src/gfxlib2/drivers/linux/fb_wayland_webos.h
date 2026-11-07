/*
 * Project: FreeBASIC Linux Wayland support
 * File: fb_wayland_webos.h
 * Purpose: Expose optional native webOS shell and text-model controls.
 * Responsibilities: Keep one surface's legacy IME and shell state together.
 * This file does not select a display driver or implement text widgets.
 *
 * All calls and callbacks run under the owning graphics driver's lock.
 * The owner must destroy this state before disconnecting its Wayland display.
 */
#ifndef FB_WAYLAND_WEBOS_H
#define FB_WAYLAND_WEBOS_H

#include "fb_wayland_client.h"

/* Covers the known protocol event tables, with room for optional events. */
#define FB_WAYLAND_WEBOS_MAX_EVENTS 16

typedef struct FB_WAYLAND_WEBOS_CALLBACKS {
	void (*text)(void *user, const char *text);
	void (*key)(void *user, uint32_t symbol, uint32_t state);
	void (*delete_before)(void *user, uint32_t count);
	void (*resize)(void *user, int width, int height);
	void (*close)(void *user);
} FB_WAYLAND_WEBOS_CALLBACKS;

typedef struct FB_WAYLAND_WEBOS {
	const FB_WAYLAND_API *api;
	struct wl_proxy *factory;
	struct wl_proxy *model;
	struct wl_proxy *shell;
	struct wl_proxy *shell_surface;
	struct wl_proxy *seat;
	struct wl_proxy *surface;
	void *user;
	FB_WAYLAND_WEBOS_CALLBACKS callbacks;
	void (*text_listener[FB_WAYLAND_WEBOS_MAX_EVENTS])(void);
	void (*shell_listener[FB_WAYLAND_WEBOS_MAX_EVENTS])(void);
	uint32_t serial;
	int entered;
	int requested;
	int text_ready;
	int visible;
	int x, y, width, height;
	int panel_top, panel_height;
	/* Duplicate shell size events must not restart the native panel. */
	int surface_width, surface_height;
	/* Completed by the panel's hidden acknowledgement, never by a timer. */
	int panel_refresh_pending;
} FB_WAYLAND_WEBOS;

void fb_hWaylandWebosGlobal(FB_WAYLAND_WEBOS *webos,
	const FB_WAYLAND_API *api, struct wl_proxy *registry, uint32_t name,
	const char *interface_name, uint32_t version);
void fb_hWaylandWebosOpen(FB_WAYLAND_WEBOS *webos, struct wl_proxy *seat,
	struct wl_proxy *surface, const char *app_id,
	const FB_WAYLAND_WEBOS_CALLBACKS *callbacks, void *user, int text_input);
int fb_hWaylandWebosKeyboard(FB_WAYLAND_WEBOS *webos, int visible);
void fb_hWaylandWebosBlur(FB_WAYLAND_WEBOS *webos);
int fb_hWaylandWebosRegion(FB_WAYLAND_WEBOS *webos,
	int x, int y, int width, int height);
void fb_hWaylandWebosDestroy(FB_WAYLAND_WEBOS *webos);

#endif

/* end of fb_wayland_webos.h */
