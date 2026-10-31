/*
 * FreeBASIC graphics: win32/gfx_metrics.h
 * Purpose: Query layout density without requiring a recent Windows import ABI.
 * Responsibilities: Per-window DPI with a balanced legacy device-context fallback.
 * This file does not change process DPI awareness or own the native window.
 */
#ifndef FB_WIN32_GFX_METRICS_H
#define FB_WIN32_GFX_METRICS_H
#include <windows.h>
static int fb_hWindowDpi(ssize_t window_handle)
{
	typedef UINT (WINAPI *GET_DPI_FOR_WINDOW)(HWND);
	HWND window = (HWND)(intptr_t)window_handle;
	HMODULE user = GetModuleHandleA("user32.dll");
	GET_DPI_FOR_WINDOW get_dpi = user ?
		(GET_DPI_FOR_WINDOW)(void *)GetProcAddress(user, "GetDpiForWindow") : NULL;
	HDC display;
	int dpi;
	if (window && get_dpi) {
		dpi = (int)get_dpi(window);
		if (dpi > 0)
			return dpi;
	}
	display = GetDC(window);
	if (!display)
		return 96;
	dpi = GetDeviceCaps(display, LOGPIXELSX);
	ReleaseDC(window, display);
	return dpi > 0 ? dpi : 96;
}
#endif
/* end of gfx_metrics.h */
