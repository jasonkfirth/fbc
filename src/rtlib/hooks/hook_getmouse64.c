/*
 * Project: FreeBASIC runtime library
 * File: hooks/hook_getmouse64.c
 * Purpose: Provide hook getmouse64 runtime support.
 * Responsibilities: Dispatch between console services and installed graphics hooks.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_GetMouse64( long long *x, long long *y, long long *z, long long *buttons, long long *clip )
{
	int ix = -1, iy = -1, iz = -1, ibuttons = -1, iclip = -1;

	int res = fb_GetMouse( &ix, &iy, &iz, &ibuttons, &iclip );

	if (x) *x = ix;
	if (y) *y = iy;
	if (z) *z = iz;
	if (buttons) *buttons = ibuttons;
	if (clip) *clip = iclip;

	return res;
}

/* end of hooks/hook_getmouse64.c */
