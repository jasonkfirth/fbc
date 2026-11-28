/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/fb_private_scancodes_x11.h
 * Purpose: Provide fb private scancodes x11 runtime support.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#ifndef FB_PRIVATE_SCANCODES_X11_H
#define FB_PRIVATE_SCANCODES_X11_H

#ifndef DISABLE_X11

#include <X11/Xlib.h>
#include <X11/keysym.h>

typedef Display *(*XOPENDISPLAY)(const char *);
typedef int (*XCLOSEDISPLAY)(Display *);
typedef int (*XQUERYKEYMAP)(Display *, char *);
typedef int (*XDISPLAYKEYCODES)(Display *, int *, int *);
#if defined(HOST_SOLARIS) || defined(HOST_DRAGONFLY)
typedef KeySym* (*XGETKEYBOARDMAPPING)(Display *, unsigned int, int, int *);
#else
typedef KeySym* (*XGETKEYBOARDMAPPING)(Display *, KeyCode, int, int *);
#endif
typedef int (*XFREE)(void *);

extern unsigned char fb_x11keycode_to_scancode[256];

void fb_hInitX11KeycodeToScancodeTb
	(
		Display *display,
		XDISPLAYKEYCODES DisplayKeycodes,
		XGETKEYBOARDMAPPING GetKeyboardMapping,
		XFREE Free
	);

#endif

#endif

/* end of drivers/unix/fb_private_scancodes_x11.h */
