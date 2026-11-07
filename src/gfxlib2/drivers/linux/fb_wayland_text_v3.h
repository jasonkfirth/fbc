/*
 * Project: FreeBASIC Linux Wayland support
 * File: fb_wayland_text_v3.h
 * Purpose: Hold one pending Wayland text-input-v3 edit.
 * Responsibilities: Bound commit text and retain replacement lengths until done.
 * This file intentionally does not interpret application text or post keys.
 *
 * The protocol double-buffers each value until its done event. A commit can
 * contain four UTF-8 bytes per character, while the legacy graphics input
 * queue accepts at most 255 characters from one commit. Keeping that prefix
 * avoids an allocation on the Wayland event thread. An incomplete UTF-8
 * sequence at the end is ignored by the existing text decoders.
 */
#ifndef FB_WAYLAND_TEXT_V3_H
#define FB_WAYLAND_TEXT_V3_H

#include <stddef.h>
#include <stdint.h>

#define FB_WAYLAND_TEXT_V3_COMMIT_BYTES (255u * 4u)

typedef struct FB_WAYLAND_TEXT_V3_PENDING {
	char text[FB_WAYLAND_TEXT_V3_COMMIT_BYTES + 1u];
	uint32_t before;
	uint32_t after;
} FB_WAYLAND_TEXT_V3_PENDING;

static inline void fb_hWaylandTextV3Reset(FB_WAYLAND_TEXT_V3_PENDING *pending)
{
	pending->text[0] = '\0';
	pending->before = 0u;
	pending->after = 0u;
}

static inline void fb_hWaylandTextV3SetCommit(
	FB_WAYLAND_TEXT_V3_PENDING *pending, const char *text)
{
	size_t length = 0u;

	if (text != NULL) {
		while ((length < FB_WAYLAND_TEXT_V3_COMMIT_BYTES) && text[length]) {
			pending->text[length] = text[length];
			length++;
		}
	}
	pending->text[length] = '\0';
}

static inline void fb_hWaylandTextV3SetDelete(
	FB_WAYLAND_TEXT_V3_PENDING *pending, uint32_t before, uint32_t after)
{
	pending->before = before;
	pending->after = after;
}

#endif

/* end of fb_wayland_text_v3.h */
