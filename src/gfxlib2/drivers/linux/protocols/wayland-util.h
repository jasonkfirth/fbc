/*
 * Project: FreeBASIC gfxlib2 Linux Wayland support
 * File: wayland-util.h
 * Purpose: Provide the protocol metadata layout used by generated bindings.
 * Responsibilities: Match libwayland's stable wl_message and wl_interface ABI.
 * This file intentionally does not declare client functions or link symbols.
 */
#ifndef FB_WAYLAND_UTIL_H
#define FB_WAYLAND_UTIL_H

struct wl_interface;

struct wl_message {
	const char *name;
	const char *signature;
	const struct wl_interface **types;
};

struct wl_interface {
	const char *name;
	int version;
	int method_count;
	const struct wl_message *methods;
	int event_count;
	const struct wl_message *events;
};

#endif

/* end of wayland-util.h */
