/*
 * Project: FreeBASIC Linux graphics tests
 * File: wayland-text-v3-bounds.c
 * Purpose: Check the bounded pending text-input-v3 buffer.
 * Responsibilities: Exercise empty, exact-size and overlong commits.
 * This file intentionally does not connect to a compositor or post keys.
 *
 * Build and run on Linux:
 *   cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
 *      tests/gfx/wayland-text-v3-bounds.c -o /tmp/wayland-text-v3-bounds
 *   /tmp/wayland-text-v3-bounds
 */
#include "../../src/gfxlib2/drivers/linux/fb_wayland_text_v3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "wayland-text-v3-bounds: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	static const size_t lengths[] = {
		0u, 1u, FB_WAYLAND_TEXT_V3_COMMIT_BYTES - 1u,
		FB_WAYLAND_TEXT_V3_COMMIT_BYTES,
		FB_WAYLAND_TEXT_V3_COMMIT_BYTES + 1u
	};
	FB_WAYLAND_TEXT_V3_PENDING pending;
	size_t index;

	memset(&pending, 0xA5, sizeof(pending));
	fb_hWaylandTextV3Reset(&pending);
	require(pending.text[0] == '\0' && pending.before == 0u &&
		pending.after == 0u, "reset did not clear pending values");
	for (index = 0u; index < sizeof(lengths) / sizeof(lengths[0]); index++) {
		size_t length = lengths[index];
		size_t copied = length < FB_WAYLAND_TEXT_V3_COMMIT_BYTES ?
			length : FB_WAYLAND_TEXT_V3_COMMIT_BYTES;
		char *text = (char *)malloc(length + 1u);
		if (text == NULL) {
			fputs("wayland-text-v3-bounds: allocation failed\n", stderr);
			return EXIT_FAILURE;
		}
		memset(text, 'x', length);
		text[length] = '\0';
		fb_hWaylandTextV3SetCommit(&pending, text);
		require(strlen(pending.text) == copied &&
			memcmp(pending.text, text, copied) == 0,
			"commit was not copied within its bound");
		free(text);
	}
	fb_hWaylandTextV3SetDelete(&pending, 2u, 3u);
	fb_hWaylandTextV3SetDelete(&pending, 4u, 5u);
	require(pending.before == 4u && pending.after == 5u,
		"a newer deletion did not replace its previous value");
	fb_hWaylandTextV3SetCommit(&pending, NULL);
	require(pending.text[0] == '\0', "null commit did not clear the text");
	fb_hWaylandTextV3Reset(&pending);
	require(pending.before == 0u && pending.after == 0u,
		"reset did not clear deletion values");
	puts("WAYLAND_TEXT_V3_BOUNDS_PASS");
	return EXIT_SUCCESS;
}

/* end of wayland-text-v3-bounds.c */
