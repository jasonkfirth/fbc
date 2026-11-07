/*
 * Project: FreeBASIC Linux graphics tests
 * File: linux-wayland-selection.c
 * Purpose: Check Wayland selection and application identity on Linux.
 * Responsibilities: Exercise launcher markers, desktop IDs and X11 fallback.
 * This file intentionally does NOT open a display or launch an application.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/linux-wayland-selection.c -Wl,--gc-sections \
 *      -o /tmp/linux-wayland-selection
 *   /tmp/linux-wayland-selection
 */
#include "../../src/gfxlib2/drivers/linux/fb_wayland_client.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_value(const char *name, const char *value)
{
	if (setenv(name, value, 1) != 0) {
		perror("setenv");
		exit(EXIT_FAILURE);
	}
}

static void clear_value(const char *name)
{
	if (unsetenv(name) != 0) {
		perror("unsetenv");
		exit(EXIT_FAILURE);
	}
}

static void expect(int actual, int expected, const char *message)
{
	if (actual != expected) {
		fprintf(stderr, "linux-wayland-selection: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	char app_id[FB_WAYLAND_APP_ID_CAPACITY];
	char long_id[FB_WAYLAND_APP_ID_CAPACITY + 1];
	const char *fallback = "freebasic";

	clear_value("MIR_SOCKET");
	clear_value("DISPLAY");
	clear_value("DESKTOP_FILE_HINT");
	clear_value("APP_ID");
	expect(fb_hWaylandShouldUse(), 1, "ordinary Wayland session rejected");
	expect(strcmp(fb_hWaylandAppId(app_id, sizeof(app_id), fallback), fallback) == 0, 1,
		"missing launcher identity did not use the fallback");

	set_value("MIR_SOCKET", "/run/user/32011/mir_socket");
	set_value("DISPLAY", ":1");
	expect(fb_hWaylandShouldUse(), 0, "unregistered Mir shell lost X11 fallback");

	set_value("DESKTOP_FILE_HINT", "example_example_1.0");
	expect(fb_hWaylandShouldUse(), 1, "registered desktop hint rejected");
	expect(strcmp(fb_hWaylandAppId(app_id, sizeof(app_id), fallback),
		"example_example_1.0") == 0, 1, "desktop ID changed");
	set_value("APP_ID", "different-app");
	set_value("DESKTOP_FILE_HINT", "/usr/share/applications/org.example.App.desktop");
	expect(strcmp(fb_hWaylandAppId(app_id, sizeof(app_id), fallback),
		"org.example.App") == 0, 1, "desktop basename or precedence is wrong");
	clear_value("DESKTOP_FILE_HINT");
	set_value("APP_ID", "example_example_1.0");
	expect(fb_hWaylandShouldUse(), 0, "application ID alone bypassed the fallback");
	expect(strcmp(fb_hWaylandAppId(app_id, sizeof(app_id), fallback),
		"example_example_1.0") == 0, 1, "application ID fallback was ignored");
	set_value("DESKTOP_FILE_HINT", "invalid name.desktop");
	expect(strcmp(fb_hWaylandAppId(app_id, sizeof(app_id), fallback),
		"example_example_1.0") == 0, 1, "invalid desktop hint was accepted");
	memset(long_id, 'a', sizeof(long_id) - 1);
	long_id[sizeof(long_id) - 1] = '\0';
	set_value("DESKTOP_FILE_HINT", long_id);
	clear_value("APP_ID");
	expect(strcmp(fb_hWaylandAppId(app_id, sizeof(app_id), fallback), fallback) == 0, 1,
		"oversized application ID was truncated");
	expect(strcmp(fb_hWaylandAppId(NULL, 0, fallback), fallback) == 0, 1,
		"missing output buffer did not use the fallback");

	set_value("APP_ID", "");
	set_value("DESKTOP_FILE_HINT", "");
	expect(fb_hWaylandShouldUse(), 0, "empty registration treated as valid");
	set_value("MIR_SOCKET", "");
	expect(fb_hWaylandShouldUse(), 1, "empty Mir socket changed desktop selection");

	puts("LINUX_WAYLAND_SELECTION_PASS");
	return EXIT_SUCCESS;
}

/* end of linux-wayland-selection.c */
