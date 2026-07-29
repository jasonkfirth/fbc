/*
    Project: FreeBASIC gfxlib3 tests
    --------------------------------

    File: vulkan-presentation.c

    Purpose:

        Verify the real Win32 or X11 Vulkan swapchain and GPU page-conversion
        path.

    Responsibilities:

        - create a native window and windowed Vulkan runtime
        - present exact 8-bit palette, RGB565, and 32-bit quadrant patterns
        - read displayed client pixels back through the native window system
        - exercise multiple in-flight frames and orderly WSI teardown

    This file intentionally does NOT contain:

        - FreeBASIC API compatibility checks
        - OpenGL or null-backend coverage
        - assumptions about a Vulkan SDK installation
*/

#include "../../src/gfxlib3/gfx3_vulkan.h"

#define PRESENTATION_RGB(red, green, blue) \
	((((uint32_t)(red)) << 16) | (((uint32_t)(green)) << 8) | \
	 ((uint32_t)(blue)))

#if defined(HOST_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define PRESENTATION_CLASS "FreeBASIC-gfxlib3-Vulkan-test"

typedef struct FB_GFX3_PRESENTATION_WINDOW {
	HWND window;
} FB_GFX3_PRESENTATION_WINDOW;

static LRESULT CALLBACK presentation_window_proc(HWND window, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	if (message == WM_CLOSE) {
		DestroyWindow(window);
		return 0;
	}
	return DefWindowProcA(window, message, wparam, lparam);
}

static int presentation_create_window(FB_GFX3_PRESENTATION_WINDOW *destination)
{
	WNDCLASSA window_class;
	RECT window_rect;
	int window_width;
	int window_height;

	if (destination == NULL)
		return FB_GFX3_INVALID;
	memset(destination, 0, sizeof(*destination));
	memset(&window_class, 0, sizeof(window_class));
	window_class.style = CS_OWNDC;
	window_class.lpfnWndProc = presentation_window_proc;
	window_class.hInstance = GetModuleHandleA(NULL);
	window_class.lpszClassName = PRESENTATION_CLASS;
	if ((RegisterClassA(&window_class) == 0) &&
	    (GetLastError() != ERROR_CLASS_ALREADY_EXISTS))
		return FB_GFX3_FAILED;
	window_rect.left = 0;
	window_rect.top = 0;
	window_rect.right = 8;
	window_rect.bottom = 8;
	if (!AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE))
		return FB_GFX3_FAILED;
	window_width = window_rect.right - window_rect.left;
	window_height = window_rect.bottom - window_rect.top;
	destination->window = CreateWindowExA(0, PRESENTATION_CLASS,
		"gfxlib3 Vulkan presentation test", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
		NULL, NULL, GetModuleHandleA(NULL), NULL);
	return (destination->window != NULL) ? FB_GFX3_OK : FB_GFX3_FAILED;
}

static void presentation_pump_messages(FB_GFX3_PRESENTATION_WINDOW *window)
{
	MSG message;

	while (PeekMessageA(&message, window->window, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

static void presentation_native_handles(FB_GFX3_PRESENTATION_WINDOW *window,
	uintptr_t *native_instance, uintptr_t *native_window)
{
	*native_instance = (uintptr_t)GetModuleHandleA(NULL);
	*native_window = (uintptr_t)window->window;
}

static int presentation_resize_client(FB_GFX3_PRESENTATION_WINDOW *window,
	uint32_t width, uint32_t height)
{
	RECT window_rect;

	window_rect.left = 0;
	window_rect.top = 0;
	window_rect.right = (LONG)width;
	window_rect.bottom = (LONG)height;
	if (!AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE))
		return FB_GFX3_FAILED;
	if (!SetWindowPos(window->window, NULL, 0, 0,
	    window_rect.right - window_rect.left,
	    window_rect.bottom - window_rect.top,
	    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER))
		return FB_GFX3_FAILED;
	presentation_pump_messages(window);
	return FB_GFX3_OK;
}

static int presentation_check_pixels(FB_GFX3_PRESENTATION_WINDOW *window,
	const uint32_t *expected)
{
	POINT positions[4];
	RECT client;
	HDC device_context;
	COLORREF actual;
	int index;
	int failed = FALSE;

	uint32_t converted;

	if (!GetClientRect(window->window, &client) || (client.right < 2) ||
	    (client.bottom < 2))
		return FB_GFX3_FAILED;
	positions[0].x = client.right / 4;
	positions[0].y = client.bottom / 4;
	positions[1].x = (client.right * 3) / 4;
	positions[1].y = client.bottom / 4;
	positions[2].x = client.right / 4;
	positions[2].y = (client.bottom * 3) / 4;
	positions[3].x = (client.right * 3) / 4;
	positions[3].y = (client.bottom * 3) / 4;
	device_context = GetDC(window->window);
	if (device_context == NULL)
		return FB_GFX3_FAILED;
	for (index = 0; index < 4; index++) {
		actual = GetPixel(device_context, positions[index].x,
			positions[index].y);
		converted = PRESENTATION_RGB(GetRValue(actual), GetGValue(actual),
			GetBValue(actual));
		if (converted != expected[index]) {
			fprintf(stderr,
				"Vulkan presentation pixel %d: got %06X, expected %06X\n",
				index, converted, expected[index]);
			failed = TRUE;
		}
	}
	ReleaseDC(window->window, device_context);
	return failed ? FB_GFX3_FAILED : FB_GFX3_OK;
}

static int presentation_check_pixel(FB_GFX3_PRESENTATION_WINDOW *window,
	int x, int y, uint32_t expected)
{
	HDC device_context;
	COLORREF actual;
	uint32_t converted;

	device_context = GetDC(window->window);
	if (device_context == NULL)
		return FB_GFX3_FAILED;
	actual = GetPixel(device_context, x, y);
	ReleaseDC(window->window, device_context);
	converted = PRESENTATION_RGB(GetRValue(actual), GetGValue(actual),
		GetBValue(actual));
	if (converted == expected)
		return FB_GFX3_OK;
	fprintf(stderr,
		"Vulkan presentation pixel (%d, %d): got %06X, expected %06X\n",
		x, y, converted, expected);
	return FB_GFX3_FAILED;
}

static void presentation_show_window(FB_GFX3_PRESENTATION_WINDOW *window)
{
	ShowWindow(window->window, SW_SHOW);
	UpdateWindow(window->window);
	presentation_pump_messages(window);
}

static void presentation_wait(FB_GFX3_PRESENTATION_WINDOW *window)
{
	GdiFlush();
	Sleep(100);
	presentation_pump_messages(window);
}

static void presentation_destroy_window(FB_GFX3_PRESENTATION_WINDOW *window)
{
	if ((window != NULL) && (window->window != NULL))
		DestroyWindow(window->window);
}

#elif defined(HOST_LINUX) && !defined(DISABLE_X11)

#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct FB_GFX3_PRESENTATION_WINDOW {
	Display *display;
	int screen;
	Window window;
} FB_GFX3_PRESENTATION_WINDOW;

static int presentation_create_window(FB_GFX3_PRESENTATION_WINDOW *destination)
{
	if (destination == NULL)
		return FB_GFX3_INVALID;
	memset(destination, 0, sizeof(*destination));
	destination->display = XOpenDisplay(NULL);
	if (destination->display == NULL)
		return FB_GFX3_UNSUPPORTED;
	destination->screen = DefaultScreen(destination->display);
	destination->window = XCreateSimpleWindow(destination->display,
		RootWindow(destination->display, destination->screen), 0, 0, 8, 8,
		0, BlackPixel(destination->display, destination->screen),
		BlackPixel(destination->display, destination->screen));
	if (destination->window == None) {
		XCloseDisplay(destination->display);
		memset(destination, 0, sizeof(*destination));
		return FB_GFX3_FAILED;
	}
	XStoreName(destination->display, destination->window,
		"gfxlib3 Vulkan presentation test");
	XSelectInput(destination->display, destination->window,
		StructureNotifyMask);
	XMapRaised(destination->display, destination->window);
	XSync(destination->display, False);
	return FB_GFX3_OK;
}

static void presentation_pump_messages(FB_GFX3_PRESENTATION_WINDOW *window)
{
	XEvent event;

	while (XPending(window->display))
		XNextEvent(window->display, &event);
}

static void presentation_native_handles(FB_GFX3_PRESENTATION_WINDOW *window,
	uintptr_t *native_instance, uintptr_t *native_window)
{
	*native_instance = (uintptr_t)window->display;
	*native_window = (uintptr_t)window->window;
}

static int presentation_resize_client(FB_GFX3_PRESENTATION_WINDOW *window,
	uint32_t width, uint32_t height)
{
	XWindowAttributes attributes;
	int attempt;

	XResizeWindow(window->display, window->window, width, height);
	XSync(window->display, False);
	for (attempt = 0; attempt < 100; attempt++) {
		if (XGetWindowAttributes(window->display, window->window,
		    &attributes) && (attributes.width == (int)width) &&
		    (attributes.height == (int)height)) {
			presentation_pump_messages(window);
			return FB_GFX3_OK;
		}
		usleep(10000);
	}
	return FB_GFX3_FAILED;
}

static unsigned int presentation_component(unsigned long pixel,
	unsigned long mask)
{
	unsigned int shift = 0;
	unsigned long maximum;
	unsigned long value;

	if (mask == 0)
		return 0;
	while (((mask >> shift) & 1u) == 0)
		shift++;
	maximum = mask >> shift;
	value = (pixel & mask) >> shift;
	return (unsigned int)((value * 255u + (maximum / 2u)) / maximum);
}

static int presentation_check_pixels(FB_GFX3_PRESENTATION_WINDOW *window,
	const uint32_t *expected)
{
	XWindowAttributes attributes;
	XImage *image;
	int positions[4][2];
	int index;
	int failed = FALSE;

	XSync(window->display, False);
	if (!XGetWindowAttributes(window->display, window->window, &attributes) ||
	    (attributes.width < 2) || (attributes.height < 2) ||
	    (attributes.visual == NULL))
		return FB_GFX3_FAILED;
	positions[0][0] = attributes.width / 4;
	positions[0][1] = attributes.height / 4;
	positions[1][0] = (attributes.width * 3) / 4;
	positions[1][1] = attributes.height / 4;
	positions[2][0] = attributes.width / 4;
	positions[2][1] = (attributes.height * 3) / 4;
	positions[3][0] = (attributes.width * 3) / 4;
	positions[3][1] = (attributes.height * 3) / 4;
	image = XGetImage(window->display, window->window, 0, 0,
		(unsigned int)attributes.width, (unsigned int)attributes.height,
		AllPlanes, ZPixmap);
	if (image == NULL)
		return FB_GFX3_FAILED;
	for (index = 0; index < 4; index++) {
		unsigned long pixel = XGetPixel(image, positions[index][0],
			positions[index][1]);
		uint32_t converted = PRESENTATION_RGB(
			presentation_component(pixel, attributes.visual->red_mask),
			presentation_component(pixel, attributes.visual->green_mask),
			presentation_component(pixel, attributes.visual->blue_mask));

		if (converted != expected[index]) {
			fprintf(stderr,
				"Vulkan presentation pixel %d: got %06X, expected %06X\n",
				index, converted, expected[index]);
			failed = TRUE;
		}
	}
	if (failed) {
		int x;
		int y;

		fprintf(stderr, "Captured X11 client pixels:\n");
		for (y = 0; y < attributes.height; y++) {
			for (x = 0; x < attributes.width; x++) {
				unsigned long pixel = XGetPixel(image, x, y);
				uint32_t converted = PRESENTATION_RGB(
					presentation_component(pixel,
					 attributes.visual->red_mask),
					presentation_component(pixel,
					 attributes.visual->green_mask),
					presentation_component(pixel,
					 attributes.visual->blue_mask));

				fprintf(stderr, "%06X%c", converted,
					(x + 1 == attributes.width) ? '\n' : ' ');
			}
		}
	}
	XDestroyImage(image);
	return failed ? FB_GFX3_FAILED : FB_GFX3_OK;
}

static int presentation_check_pixel(FB_GFX3_PRESENTATION_WINDOW *window,
	int x, int y, uint32_t expected)
{
	XWindowAttributes attributes;
	XImage *image;
	unsigned long pixel;
	uint32_t converted;

	if (!XGetWindowAttributes(window->display, window->window, &attributes))
		return FB_GFX3_FAILED;
	image = XGetImage(window->display, window->window, 0, 0,
		(unsigned int)attributes.width, (unsigned int)attributes.height,
		AllPlanes, ZPixmap);
	if ((image == NULL) || (x < 0) || (y < 0) ||
	    (x >= attributes.width) || (y >= attributes.height)) {
		if (image != NULL)
			XDestroyImage(image);
		return FB_GFX3_FAILED;
	}
	pixel = XGetPixel(image, x, y);
	converted = PRESENTATION_RGB(
		presentation_component(pixel, attributes.visual->red_mask),
		presentation_component(pixel, attributes.visual->green_mask),
		presentation_component(pixel, attributes.visual->blue_mask));
	XDestroyImage(image);
	if (converted == expected)
		return FB_GFX3_OK;
	fprintf(stderr,
		"Vulkan presentation pixel (%d, %d): got %06X, expected %06X\n",
		x, y, converted, expected);
	return FB_GFX3_FAILED;
}

static void presentation_show_window(FB_GFX3_PRESENTATION_WINDOW *window)
{
	XMapRaised(window->display, window->window);
	XSync(window->display, False);
	presentation_pump_messages(window);
}

static void presentation_wait(FB_GFX3_PRESENTATION_WINDOW *window)
{
	XSync(window->display, False);
	usleep(100000);
	presentation_pump_messages(window);
}

static void presentation_destroy_window(FB_GFX3_PRESENTATION_WINDOW *window)
{
	if ((window == NULL) || (window->display == NULL))
		return;
	if (window->window != None)
		XDestroyWindow(window->display, window->window);
	XCloseDisplay(window->display);
	memset(window, 0, sizeof(*window));
}

#endif

#if defined(HOST_WIN32) || \
	(defined(HOST_LINUX) && !defined(DISABLE_X11))

static int presentation_present_and_check(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const uint32_t *palette,
	FB_GFX3_PRESENTATION_WINDOW *window, const uint32_t *expected)
{
	int result;

	result = fb_gfx3_vulkan_surface_present(runtime, surface, palette, 256,
		NULL, 0);
	if (result != FB_GFX3_OK)
		return result;
	presentation_show_window(window);
	result = fb_gfx3_vulkan_surface_present(runtime, surface, palette, 256,
		NULL, 0);
	if (result != FB_GFX3_OK)
		return result;
	presentation_wait(window);
	return presentation_check_pixels(window, expected);
}

static int presentation_present_keyboard_overlay(
	FB_GFX3_VULKAN_RUNTIME *runtime, FB_GFX3_VULKAN_SURFACE *surface,
	const uint32_t *palette, FB_GFX3_PRESENTATION_WINDOW *window,
	const int32_t keyboard_button_rect[4], uint32_t keyboard_button_state)
{
	int result;

	result = fb_gfx3_vulkan_surface_present(runtime, surface, palette, 256,
		keyboard_button_rect, keyboard_button_state);
	if (result != FB_GFX3_OK)
		return result;
	presentation_show_window(window);
	result = fb_gfx3_vulkan_surface_present(runtime, surface, palette, 256,
		keyboard_button_rect, keyboard_button_state);
	if (result != FB_GFX3_OK)
		return result;
	presentation_wait(window);
	return FB_GFX3_OK;
}

static int presentation_test_surface(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, uint32_t depth, const void *pixels,
	size_t pitch, const uint32_t *palette,
	FB_GFX3_PRESENTATION_WINDOW *window, const uint32_t *expected)
{
	int result;

	fb_gfx3_vulkan_surface_destroy(runtime, surface);
	result = fb_gfx3_vulkan_surface_create(runtime, surface, 2, 2, depth, 0);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vulkan_surface_upload(runtime, surface, 0, 0, 2, 2,
		pixels, pitch);
	if (result != FB_GFX3_OK)
		return result;
	return presentation_present_and_check(runtime, surface, palette, window,
		expected);
}

int main(void)
{
	static const uint32_t expected[4] = {
		PRESENTATION_RGB(255, 0, 0), PRESENTATION_RGB(0, 255, 0),
		PRESENTATION_RGB(0, 0, 255), PRESENTATION_RGB(255, 255, 255)
	};
	static const uint32_t pixels32[4] = {
		0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0x00FFFFFFu
	};
	static const uint16_t pixels16[4] = {
		0xF800u, 0x07E0u, 0x001Fu, 0xFFFFu
	};
	static const unsigned char pixels1[4] = { 0, 1, 1, 0 };
	static const unsigned char pixels2[4] = { 1, 2, 3, 0 };
	static const unsigned char pixels4[4] = {
		0x11u, 0x22u, 0x33u, 0x44u
	};
	static const unsigned char pixels8[4] = { 1, 2, 3, 4 };
	static const uint32_t expected1[4] = {
		PRESENTATION_RGB(255, 0, 0), PRESENTATION_RGB(0, 255, 0),
		PRESENTATION_RGB(0, 255, 0), PRESENTATION_RGB(255, 0, 0)
	};
	static const int32_t keyboard_button_rect[4] = { 8, 8, 64, 48 };
	FB_GFX3_VULKAN_RUNTIME runtime;
	FB_GFX3_VULKAN_SURFACE surface;
	FB_GFX3_PRESENTATION_WINDOW window;
	uint32_t palette[256];
	uintptr_t native_instance;
	uintptr_t native_window;
	int result = FB_GFX3_FAILED;

	memset(&runtime, 0, sizeof(runtime));
	memset(&surface, 0, sizeof(surface));
	memset(&window, 0, sizeof(window));
	memset(palette, 0, sizeof(palette));
	palette[1] = 0x00FF0000u;
	palette[2] = 0x0000FF00u;
	palette[3] = 0x000000FFu;
	palette[4] = 0x00FFFFFFu;
	result = presentation_create_window(&window);
	if (result != FB_GFX3_OK)
		goto cleanup;
	presentation_native_handles(&window, &native_instance, &native_window);
	result = fb_gfx3_vulkan_runtime_open_windowed(&runtime,
		native_instance, native_window, 8, 8);
	if (result == FB_GFX3_UNSUPPORTED) {
		fprintf(stderr,
			"Vulkan presentation: unsupported on this machine\n");
		result = FB_GFX3_OK;
		goto cleanup;
	}
	if ((result != FB_GFX3_OK) || !runtime.windowed ||
	    (runtime.present_width != 8) || (runtime.present_height != 8) ||
	    (runtime.swapchain_image_count < 2))
		goto cleanup;
	result = presentation_test_surface(&runtime, &surface, 32, pixels32,
		sizeof(uint32_t) * 2, palette, &window, expected);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = presentation_test_surface(&runtime, &surface, 16, pixels16,
		sizeof(uint16_t) * 2, palette, &window, expected);
	if (result != FB_GFX3_OK)
		goto cleanup;
	palette[0] = 0x00FF0000u;
	palette[1] = 0x0000FF00u;
	result = presentation_test_surface(&runtime, &surface, 1, pixels1,
		sizeof(unsigned char) * 2, palette, &window, expected1);
	if (result != FB_GFX3_OK)
		goto cleanup;
	palette[0] = 0x00FFFFFFu;
	palette[1] = 0x00FF0000u;
	palette[2] = 0x0000FF00u;
	palette[3] = 0x000000FFu;
	result = presentation_test_surface(&runtime, &surface, 2, pixels2,
		sizeof(unsigned char) * 2, palette, &window, expected);
	if (result != FB_GFX3_OK)
		goto cleanup;
	palette[0] = 0;
	palette[4] = 0x00FFFFFFu;
	result = presentation_test_surface(&runtime, &surface, 4, pixels4,
		sizeof(unsigned char) * 2, palette, &window, expected);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = presentation_test_surface(&runtime, &surface, 8, pixels8,
		sizeof(unsigned char) * 2, palette, &window, expected);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = presentation_resize_client(&window, 16, 12);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = fb_gfx3_vulkan_runtime_resize(&runtime, 16, 12);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = presentation_present_and_check(&runtime, &surface, palette,
		&window, expected);
	if ((result != FB_GFX3_OK) || (runtime.present_width != 16) ||
	    (runtime.present_height != 12)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	result = presentation_resize_client(&window, 72, 64);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = fb_gfx3_vulkan_runtime_resize(&runtime, 72, 64);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = presentation_present_keyboard_overlay(&runtime, &surface, palette,
		&window, keyboard_button_rect, 2);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = presentation_check_pixel(&window, 12, 12,
		PRESENTATION_RGB(40, 120, 180));
	if (result != FB_GFX3_OK)
		goto cleanup;
	if (runtime.maximum_in_flight_submission_count < 2u) {
		fprintf(stderr,
			"Vulkan presentation did not retain multiple in-flight frames\n");
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	fprintf(stderr,
		"Vulkan presentation: formats and keyboard compositor passed\n");
	result = FB_GFX3_OK;

cleanup:
	fb_gfx3_vulkan_surface_destroy(&runtime, &surface);
	fb_gfx3_vulkan_runtime_close(&runtime);
	presentation_destroy_window(&window);
	return (result == FB_GFX3_OK) ? 0 : 1;
}

#else

int main(void)
{
	fprintf(stderr, "Vulkan presentation: unsupported on this platform\n");
	return 0;
}

#endif

/* end of vulkan-presentation.c */
