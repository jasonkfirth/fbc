/*
 * Project: FreeBASIC gfxlib2 X11 tests
 * File: x11-dpi-resource.c
 * Purpose: Observe DPI resource changes on an already-open display connection.
 * Responsibilities: Check live values and invalid or oversized resource fallback.
 * This file does not alter a user's desktop; run only on a private test X server.
 *
 * Build with section garbage collection, -lX11 and -lm. The runner must set
 * FBC_X11_PRIVATE_TEST=1 and DISPLAY to its own disposable X server.
 */
#include "../../src/gfxlib2/drivers/linux/gfx_mobile.c"
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>

static void put_resource(Display *display, const char *text, int length)
{
	XChangeProperty(display, DefaultRootWindow(display), XA_RESOURCE_MANAGER,
		XA_STRING, 8, PropModeReplace, (const unsigned char *)text, length);
	XSync(display, False);
}

int main(void)
{
	Display *writer, *reader;
	const char *private_test = getenv("FBC_X11_PRIVATE_TEST");
	const char *initial = "Xft.dpi: 96\n";
	const char *changed = "Xft.dpi: 240\n";
	/* One byte beyond the supported 64 KiB resource-property bound. */
	char oversized[65537];
	int failed = 0, dpi;
	if (!private_test || strcmp(private_test, "1")) {
		fputs("x11-dpi-resource: a private X server is required\n", stderr);
		return EXIT_FAILURE;
	}
	writer = XOpenDisplay(NULL);
	if (!writer)
		return EXIT_FAILURE;
	put_resource(writer, initial, (int)strlen(initial));
	/* XOpenDisplay caches RESOURCE_MANAGER. Keep this connection alive while
	 * another client changes the property, as a desktop settings tool does. */
	reader = XOpenDisplay(NULL);
	if (!reader) {
		XCloseDisplay(writer);
		return EXIT_FAILURE;
	}
	if (linux_display_dpi(reader, DefaultScreen(reader)) != 96)
		failed = 1;
	put_resource(writer, changed, (int)strlen(changed));
	dpi = linux_display_dpi(reader, DefaultScreen(reader));
	if (dpi != 240) {
		fprintf(stderr, "x11-dpi-resource: live DPI stayed at %d instead of 240\n", dpi);
		failed = 1;
	}
	put_resource(writer, "Xft.dpi: nan\n", 13);
	dpi = linux_display_dpi(reader, DefaultScreen(reader));
	if (dpi < 72 || dpi > 768 || dpi == 240)
		failed = 1;
	memset(oversized, ' ', sizeof(oversized));
	memcpy(oversized, changed, strlen(changed));
	put_resource(writer, oversized, sizeof(oversized));
	dpi = linux_display_dpi(reader, DefaultScreen(reader));
	if (dpi < 72 || dpi > 768 || dpi == 240)
		failed = 1;
	XCloseDisplay(reader);
	XCloseDisplay(writer);
	if (failed)
		return EXIT_FAILURE;
	puts("X11_LIVE_DPI=PASS");
	return EXIT_SUCCESS;
}

/* end of x11-dpi-resource.c */
