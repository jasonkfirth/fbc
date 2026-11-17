/*
 * FreeBASIC graphics: win32/gfx_metrics.h
 * Purpose: Query layout density without requiring a recent Windows import ABI.
 * Responsibilities: Per-window DPI with a balanced legacy device-context fallback.
 * This file does not change process DPI awareness or own the native window.
 */
#ifndef FB_WIN32_GFX_METRICS_H
#define FB_WIN32_GFX_METRICS_H
#include <windows.h>
#include <string.h>
static int fb_hWindowDpi(ssize_t window_handle)
{
	typedef UINT (WINAPI *GET_DPI_FOR_WINDOW)(HWND);
	/* GetProcAddress returns FARPROC for every symbol; Win32 function
	   pointers share one address representation. */
	_Static_assert(sizeof(FARPROC) == sizeof(GET_DPI_FOR_WINDOW),
		"Win32 function pointer size");
	HWND window = (HWND)(intptr_t)window_handle;
	HMODULE user = GetModuleHandleA("user32.dll");
	FARPROC get_dpi_proc = user ?
		GetProcAddress(user, "GetDpiForWindow") : NULL;
	GET_DPI_FOR_WINDOW get_dpi = NULL;
	HDC display;
	int dpi;
	if (get_dpi_proc != NULL)
		memcpy((void *)&get_dpi, (const void *)&get_dpi_proc, sizeof(get_dpi));
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
