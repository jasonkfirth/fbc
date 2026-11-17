/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_xfocus.c
 * Purpose: XTerm focus query helpers.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "../../fb_private_xfocus.h"

#ifndef DISABLE_X11
#include "../../fb_private_hdynload.h"
#include <X11/Xlib.h>
#include <limits.h>

typedef Display *(*XOPENDISPLAY)(const char *);
typedef int (*XCLOSEDISPLAY)(Display *);
typedef int (*XGETINPUTFOCUS)(Display *, Window *, int *);

typedef struct {
	XOPENDISPLAY OpenDisplay;
	XCLOSEDISPLAY CloseDisplay;
	XGETINPUTFOCUS GetInputFocus;
} X_FUNCS;

/* Keyboard and mouse retain balanced attempts under the background lock.
   A failed attempt keeps its reference so either owner may release first. */
static unsigned int ref_count = 0;
static FB_DYLIB xlib;
static X_FUNCS X = { NULL, NULL, NULL };
static Display *display;
static Window xterm_window;
#endif

int fb_hXTermInitFocus(void)
{
#ifndef DISABLE_X11
	const char *const funcs[] = { "XOpenDisplay", "XCloseDisplay", "XGetInputFocus", NULL };
	int dummy;

	if (ref_count == UINT_MAX)
		return -1;
	++ref_count;
	if (display != NULL)
		return 0;

	xlib = fb_hDynLoad("libX11.so", funcs, (void **)&X);
	if (!xlib)
		return -1;

	display = X.OpenDisplay(NULL);
	if (!display) {
		fb_hDynUnload(&xlib);
		X = (X_FUNCS){ NULL, NULL, NULL };
		return -1;
	}

	X.GetInputFocus(display, &xterm_window, &dummy);
#endif
	return 0;
}

void fb_hXTermExitFocus(void)
{
#ifndef DISABLE_X11
	if (ref_count == 0)
		return;
	--ref_count;
	if (ref_count != 0)
		return;
	if (display != NULL) {
		X.CloseDisplay(display);
		display = NULL;
	}
	if (xlib != NULL)
		fb_hDynUnload(&xlib);
	X = (X_FUNCS){ NULL, NULL, NULL };
#endif
}

int fb_hXTermHasFocus(void)
{
#ifndef DISABLE_X11
	Window focus_window;
	int dummy;

	if (display == NULL)
		return FALSE;

	X.GetInputFocus(display, &focus_window, &dummy);

	return (focus_window == xterm_window);
#else
	return 0;
#endif
}

/* end of drivers/unix/io_xfocus.c */
