/*
 * Project: FreeBASIC Linux graphics support
 * File: fb_linux_display.c
 * Purpose: Read bounded display geometry from the Linux framebuffer sysfs ABI.
 * Responsibilities: Report virtual size and optional quarter-turn rotation.
 * This file intentionally does NOT open X11/Wayland connections or choose UI.
 *
 * Mir sessions may expose XWayland through DISPLAY while a fresh X11 client
 * connection is temporarily unable to complete. The framebuffer sysfs values
 * provide a local, nonblocking fallback for display metrics before a window
 * exists. Unsupported or malformed values remain unavailable to the caller.
 */
#define _POSIX_C_SOURCE 200809L

#include "fb_linux_display.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define FB_LINUX_DISPLAY_MAX_SIDE 8192
#define FB_LINUX_DISPLAY_MAX_PIXELS 8388608L

static int fb_hLinuxDisplayAsciiSpace(unsigned char value)
{
	return (value == ' ') || (value == '\t') || (value == '\n') ||
		(value == '\r') || (value == '\f') || (value == '\v');
}

int fb_hLinuxFramebufferMetrics(ssize_t *width, ssize_t *height,
	ssize_t *dpi, ssize_t *rotation)
{
	FILE *file;
	char value[64];
	char *end;
	char *height_start;
	long parsed_width;
	long parsed_height;
	long parsed_rotation = 0;

	if (width == NULL || height == NULL)
		return 0;
	*width = 0;
	*height = 0;
	if (dpi)
		*dpi = 96;
	if (rotation)
		*rotation = 0;

	file = fopen("/sys/class/graphics/fb0/virtual_size", "r");
	if (file == NULL)
		return 0;
	if (fgets(value, sizeof(value), file) == NULL) {
		fclose(file);
		return 0;
	}
	fclose(file);

	errno = 0;
	parsed_width = strtol(value, &end, 10);
	if ((errno != 0) || (end == value) || (*end != ','))
		return 0;
	height_start = end + 1;
	errno = 0;
	parsed_height = strtol(height_start, &end, 10);
	while (*end && fb_hLinuxDisplayAsciiSpace((unsigned char)*end))
		end++;
	if ((errno != 0) || (end == height_start) || (*end != '\0'))
		return 0;
	if ((parsed_width < 2) || (parsed_height < 2) ||
	    (parsed_width > FB_LINUX_DISPLAY_MAX_SIDE) ||
	    (parsed_height > FB_LINUX_DISPLAY_MAX_SIDE) ||
	    (parsed_width * parsed_height > FB_LINUX_DISPLAY_MAX_PIXELS))
		return 0;

	file = fopen("/sys/class/graphics/fb0/rotate", "r");
	if (file != NULL) {
		if (fgets(value, sizeof(value), file) != NULL) {
			errno = 0;
			parsed_rotation = strtol(value, &end, 10);
			while (*end && fb_hLinuxDisplayAsciiSpace((unsigned char)*end))
				end++;
			if ((errno != 0) || (end == value) || (*end != '\0') ||
			    (parsed_rotation < 0) || (parsed_rotation > 3))
				parsed_rotation = 0;
		}
		fclose(file);
	}

	*width = (ssize_t)parsed_width;
	*height = (ssize_t)parsed_height;
	if (rotation)
		*rotation = (ssize_t)(parsed_rotation * 90);
	return 1;
}

/* end of fb_linux_display.c */
