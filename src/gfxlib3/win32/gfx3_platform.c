/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: win32/gfx3_platform.c

    Purpose:

        Own the Win32 window used by a gfxlib3 renderer and, when requested,
        its device context and WGL context.

    Responsibilities:

        - register the private Win32 window class
		- create a correctly sized native window for OpenGL or Vulkan
		- bootstrap and own an OpenGL 4.3 or newer core context
		- load WGL and OpenGL entry points without a permanent loader library
		- translate Win32 input into the common synchronized input state
		- poll optional XInput controllers into the common input snapshot
		- pump native messages and swap the window buffers

    This file intentionally does NOT contain:

        - shaders, GPU surfaces, or graphics primitive execution
		- persistent display-mode changes or arbitrary resize policy
*/

#include "../gfx3_platform.h"
#include "../gfx3_input.h"

#if defined(HOST_WIN32) && !defined(DISABLE_OPENGL)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <GL/wglext.h>

#include "../../rtlib/win32/fb_private_console.h"

#define FB_GFX3_WIN32_WINDOW_CLASS "FreeBASIC-gfxlib3"

/*
	The window pump may run once per SCREENEVENT call. Four absent-controller
	XInput queries are much more expensive than checking the Win32 message queue,
	and controller input cannot usefully change faster than a display scan. An
	8 ms cache interval retains low-latency gamepad input while avoiding repeated
	DLL calls in tight event loops.
*/
#define FB_GFX3_XINPUT_POLL_MILLISECONDS 8u

typedef HGLRC (WINAPI *FB_GFX3_WGL_CREATE_CONTEXT)(HDC device_context);
typedef BOOL (WINAPI *FB_GFX3_WGL_DELETE_CONTEXT)(HGLRC context);
typedef BOOL (WINAPI *FB_GFX3_WGL_MAKE_CURRENT)(HDC device_context,
	HGLRC context);
typedef PROC (WINAPI *FB_GFX3_WGL_GET_PROC_ADDRESS)(LPCSTR name);

/*
	XInput is deliberately declared locally and loaded at runtime.  Some
	otherwise supported Windows installations do not carry the same XInput DLL
	as a current desktop SDK, and gfxlib3 must still open a window there.  These
	definitions are the stable XInputGetState binary layout, not a dependency on
	the SDK header or import library.
*/
#define FB_GFX3_XINPUT_GAMEPAD_DPAD_UP          0x0001u
#define FB_GFX3_XINPUT_GAMEPAD_DPAD_DOWN        0x0002u
#define FB_GFX3_XINPUT_GAMEPAD_DPAD_LEFT        0x0004u
#define FB_GFX3_XINPUT_GAMEPAD_DPAD_RIGHT       0x0008u
#define FB_GFX3_XINPUT_GAMEPAD_START            0x0010u
#define FB_GFX3_XINPUT_GAMEPAD_BACK             0x0020u
#define FB_GFX3_XINPUT_GAMEPAD_LEFT_THUMB       0x0040u
#define FB_GFX3_XINPUT_GAMEPAD_RIGHT_THUMB      0x0080u
#define FB_GFX3_XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100u
#define FB_GFX3_XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200u
#define FB_GFX3_XINPUT_GAMEPAD_GUIDE            0x0400u
#define FB_GFX3_XINPUT_GAMEPAD_A                0x1000u
#define FB_GFX3_XINPUT_GAMEPAD_B                0x2000u
#define FB_GFX3_XINPUT_GAMEPAD_X                0x4000u
#define FB_GFX3_XINPUT_GAMEPAD_Y                0x8000u

typedef struct FB_GFX3_XINPUT_GAMEPAD {
	WORD buttons;
	BYTE left_trigger;
	BYTE right_trigger;
	SHORT left_x;
	SHORT left_y;
	SHORT right_x;
	SHORT right_y;
} FB_GFX3_XINPUT_GAMEPAD;

typedef struct FB_GFX3_XINPUT_STATE {
	DWORD packet_number;
	FB_GFX3_XINPUT_GAMEPAD gamepad;
} FB_GFX3_XINPUT_STATE;

typedef DWORD (WINAPI *FB_GFX3_XINPUT_GET_STATE)(DWORD index,
	FB_GFX3_XINPUT_STATE *state);

typedef struct FB_GFX3_PLATFORM_WIN32 {
	HMODULE library;
	HWND window;
	HDC device_context;
	HGLRC context;
	HMODULE xinput_library;
	FB_GFX3_INPUT_STATE *input;
	FB_GFX3_WGL_CREATE_CONTEXT wgl_create_context;
	FB_GFX3_WGL_DELETE_CONTEXT wgl_delete_context;
	FB_GFX3_WGL_MAKE_CURRENT wgl_make_current;
	FB_GFX3_WGL_GET_PROC_ADDRESS wgl_get_proc_address;
	FB_GFX3_XINPUT_GET_STATE xinput_get_state;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wgl_create_context_attributes;
	PFNWGLSWAPINTERVALEXTPROC wgl_swap_interval;
	int shown;
	int close_requested;
	int mouse_tracking;
	int mouse_buttons;
	int mouse_clip;
	int cursor_visible;
	uint32_t flags;
	uint32_t logical_width;
	uint32_t logical_height;
	int xinput_load_attempted;
	ULONGLONG next_xinput_poll;
} FB_GFX3_PLATFORM_WIN32;

/* ------------------------------------------------------------------------- */
/* Window lifecycle                                                          */
/* ------------------------------------------------------------------------- */

static int platform_win32_presentation_layout(
	FB_GFX3_PLATFORM_WIN32 *platform, FB_GFX3_PRESENTATION_LAYOUT *layout)
{
	RECT client;
	LONG width;
	LONG height;

	if ((platform == NULL) || (layout == NULL) ||
	    !GetClientRect(platform->window, &client))
		return FB_GFX3_FAILED;
	width = client.right - client.left;
	height = client.bottom - client.top;
	if ((width < 0) || (height < 0))
		return FB_GFX3_FAILED;
	return fb_gfx3_platform_presentation_layout(platform->logical_width,
		platform->logical_height, (uint32_t)width, (uint32_t)height,
		layout);
}

static void platform_win32_client_to_logical(
	FB_GFX3_PLATFORM_WIN32 *platform, int client_x, int client_y,
	int *logical_x, int *logical_y)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;

	if ((platform == NULL) || (platform->flags & FB_GFX3_WINDOW_RESIZABLE) ||
	    (platform_win32_presentation_layout(platform, &layout) !=
	     FB_GFX3_OK)) {
		if (logical_x != NULL)
			*logical_x = client_x;
		if (logical_y != NULL)
			*logical_y = client_y;
		return;
	}
	fb_gfx3_platform_client_to_logical(&layout, platform->logical_width,
		platform->logical_height, client_x, client_y, logical_x, logical_y);
}

static void platform_win32_logical_to_client(
	FB_GFX3_PLATFORM_WIN32 *platform, int logical_x, int logical_y,
	int *client_x, int *client_y)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;

	if ((platform == NULL) || (platform->flags & FB_GFX3_WINDOW_RESIZABLE) ||
	    (platform_win32_presentation_layout(platform, &layout) !=
	     FB_GFX3_OK)) {
		if (client_x != NULL)
			*client_x = logical_x;
		if (client_y != NULL)
			*client_y = logical_y;
		return;
	}
	fb_gfx3_platform_logical_to_client(&layout, platform->logical_width,
		platform->logical_height, logical_x, logical_y, client_x, client_y);
}

static void platform_win32_apply_mouse_clip(FB_GFX3_PLATFORM_WIN32 *platform)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;
	RECT client;
	POINT top_left;
	POINT bottom_right;

	if ((platform == NULL) || !platform->mouse_clip ||
	    !IsWindowEnabled(platform->window) ||
	    (GetForegroundWindow() != platform->window)) {
		ClipCursor(NULL);
		return;
	}
	if (!GetClientRect(platform->window, &client))
		return;
	if (!(platform->flags & FB_GFX3_WINDOW_RESIZABLE) &&
	    (platform_win32_presentation_layout(platform, &layout) ==
	     FB_GFX3_OK)) {
		client.left = layout.x;
		client.top = layout.y;
		client.right = layout.x + (LONG)layout.width;
		client.bottom = layout.y + (LONG)layout.height;
	}
	top_left.x = client.left;
	top_left.y = client.top;
	bottom_right.x = client.right;
	bottom_right.y = client.bottom;
	if (!ClientToScreen(platform->window, &top_left) ||
	    !ClientToScreen(platform->window, &bottom_right))
		return;
	client.left = top_left.x;
	client.top = top_left.y;
	client.right = bottom_right.x;
	client.bottom = bottom_right.y;
	ClipCursor(&client);
}

static void platform_win32_apply_mouse_request(
	FB_GFX3_PLATFORM_WIN32 *platform)
{
	FB_GFX3_MOUSE_REQUEST request;
	POINT position;
	int client_x;
	int client_y;

	if ((platform == NULL) ||
	    !fb_gfx3_input_platform_take_mouse_request(platform->input,
	     &request))
		return;
	if (request.flags & FB_GFX3_MOUSE_REQUEST_POSITION) {
		platform_win32_logical_to_client(platform, request.x, request.y,
			&client_x, &client_y);
		position.x = client_x;
		position.y = client_y;
		if (ClientToScreen(platform->window, &position))
			SetCursorPos(position.x, position.y);
	}
	if (request.flags & FB_GFX3_MOUSE_REQUEST_CURSOR) {
		platform->cursor_visible = (request.cursor != 0);
		PostMessageA(platform->window, WM_SETCURSOR,
			(WPARAM)platform->window, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
	}
	if (request.flags & FB_GFX3_MOUSE_REQUEST_CLIP) {
		platform->mouse_clip = (request.clip != 0);
		platform_win32_apply_mouse_clip(platform);
	}
}

static void platform_win32_apply_window_request(
	FB_GFX3_PLATFORM_WIN32 *platform)
{
	FB_GFX3_WINDOW_REQUEST request;
	RECT window_rect;

	if ((platform == NULL) ||
	    !fb_gfx3_input_platform_take_window_request(platform->input,
	     &request))
		return;
	if ((request.flags & FB_GFX3_WINDOW_REQUEST_POSITION) &&
	    SetWindowPos(platform->window, HWND_TOP, request.x, request.y,
	     0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE |
	     SWP_NOZORDER) && GetWindowRect(platform->window, &window_rect)) {
		fb_gfx3_input_platform_window_moved(platform->input,
			window_rect.left, window_rect.top);
	}
}

static int platform_win32_key_ascii(WPARAM virtual_key, LPARAM key_data,
	int *translated_key)
{
	BYTE key_state[256];
	WORD characters[2] = { 0, 0 };
	WORD virtual_scan = (WORD)((key_data >> 16) & 0xFF);
	DWORD control_state = 0;
	int ascii = 0;

	memset(key_state, 0, sizeof(key_state));
	GetKeyboardState(key_state);
	if ((key_state[VK_SHIFT] | key_state[VK_LSHIFT] |
	     key_state[VK_RSHIFT]) & 0x80)
		control_state |= SHIFT_PRESSED;
	if ((key_state[VK_CONTROL] | key_state[VK_LCONTROL]) & 0x80)
		control_state |= LEFT_CTRL_PRESSED;
	if (key_state[VK_RCONTROL] & 0x80)
		control_state |= RIGHT_CTRL_PRESSED;
	if ((key_state[VK_MENU] | key_state[VK_LMENU]) & 0x80)
		control_state |= LEFT_ALT_PRESSED;
	if (key_state[VK_RMENU] & 0x80)
		control_state |= RIGHT_ALT_PRESSED;
	if (key_data & 0x01000000)
		control_state |= ENHANCED_KEY;
	if ((MapVirtualKeyA((UINT)virtual_key, MAPVK_VK_TO_CHAR) &
	     0x80000000u) == 0) {
		if (ToAscii((UINT)virtual_key, virtual_scan, key_state,
		    characters, 0) == 1)
			ascii = (unsigned char)characters[0];
	}
	if (translated_key != NULL) {
		*translated_key = fb_hConsoleTranslateKey((char)ascii,
			virtual_scan, (WORD)virtual_key, control_state, FALSE);
	}
	return ascii;
}

static int platform_win32_convert_character(int character)
{
	WCHAR wide[2];
	char converted[4];
	char source = (char)character;
	int wide_length;
	int converted_length;

	if ((character <= 0) || (character >= 256))
		return 0;
	wide_length = MultiByteToWideChar(CP_ACP, 0, &source, 1, wide,
		sizeof(wide) / sizeof(wide[0]));
	if (wide_length <= 0)
		return 0;
	converted_length = WideCharToMultiByte(437, 0, wide, wide_length,
		converted, sizeof(converted), NULL, NULL);
	if (converted_length <= 0)
		return 0;
	return (unsigned char)converted[0];
}

static void platform_win32_track_mouse(FB_GFX3_PLATFORM_WIN32 *platform,
	HWND window, LPARAM position)
{
	int logical_x;
	int logical_y;

	if (!platform->mouse_tracking) {
		TRACKMOUSEEVENT tracking;

		memset(&tracking, 0, sizeof(tracking));
		tracking.cbSize = sizeof(tracking);
		tracking.dwFlags = TME_LEAVE;
		tracking.hwndTrack = window;
		if (TrackMouseEvent(&tracking))
			platform->mouse_tracking = TRUE;
		fb_gfx3_input_platform_mouse_enter(platform->input);
	}
	platform_win32_client_to_logical(platform,
		(int)(short)LOWORD(position), (int)(short)HIWORD(position),
		&logical_x, &logical_y);
	fb_gfx3_input_platform_mouse_move(platform->input, logical_x, logical_y);
}

/*
	TrackMouseEvent can deliver a queued WM_MOUSELEAVE after SETMOUSE has
	already positioned the physical cursor back in the client area.  gfxlib2
	rechecks the desktop cursor before it handles every window message, so its
	GETMOUSE result continues to describe that physical position.  Keep the
	same rule here instead of allowing an obsolete tracking notification to
	make GETMOUSE fail.
*/
static int platform_win32_mouse_in_client(HWND window)
{
	POINT position;
	RECT client;

	return GetCursorPos(&position) && ScreenToClient(window, &position) &&
		GetClientRect(window, &client) && PtInRect(&client, position);
}

static LRESULT CALLBACK platform_win32_window_proc(HWND window, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	LONG_PTR user_data = GetWindowLongPtrA(window, GWLP_USERDATA);
	FB_GFX3_PLATFORM_WIN32 *platform =
		(FB_GFX3_PLATFORM_WIN32 *)(uintptr_t)user_data;
	int button;

	if (platform == NULL)
		return DefWindowProcA(window, message, wparam, lparam);
	switch (message) {
	case WM_ACTIVATE:
		{
			int focused = (LOWORD(wparam) != WA_INACTIVE) &&
				!HIWORD(wparam);

			fb_gfx3_input_platform_focus(platform->input, focused);
			if (focused) {
				POINT position;
				RECT client;

				if (GetCursorPos(&position) &&
				    ScreenToClient(window, &position) &&
				    GetClientRect(window, &client) &&
				    PtInRect(&client, position)) {
					int logical_x;
					int logical_y;

					platform_win32_client_to_logical(platform,
						position.x, position.y, &logical_x,
						&logical_y);
					fb_gfx3_input_platform_mouse_enter(
						platform->input);
					fb_gfx3_input_platform_mouse_move(
						platform->input, logical_x, logical_y);
				}
			}
		}
		if (LOWORD(wparam) == WA_INACTIVE)
			ClipCursor(NULL);
		else if (platform->mouse_clip)
			platform_win32_apply_mouse_clip(platform);
		return 0;

	case WM_MOUSEMOVE:
		platform_win32_track_mouse(platform, window, lparam);
		return 0;

	case WM_MOVE:
		{
			RECT window_rect;

			if (GetWindowRect(window, &window_rect))
				fb_gfx3_input_platform_window_moved(platform->input,
					window_rect.left, window_rect.top);
		}
		return 0;

	case WM_SIZE:
		if (((platform->flags & FB_GFX3_WINDOW_RESIZABLE) != 0u) &&
		    (wparam != SIZE_MINIMIZED)) {
			uint32_t width = LOWORD(lparam);
			uint32_t height = HIWORD(lparam);

			if ((width != 0u) && (height != 0u))
				fb_gfx3_input_platform_resize(platform->input,
					width, height);
			if (platform->mouse_clip)
				platform_win32_apply_mouse_clip(platform);
		}
		else if ((wparam != SIZE_MINIMIZED) && platform->mouse_clip) {
			platform_win32_apply_mouse_clip(platform);
		}
		return 0;

	case WM_GETMINMAXINFO:
		if ((platform->flags & FB_GFX3_WINDOW_RESIZABLE) != 0u) {
			MINMAXINFO *limits = (MINMAXINFO *)lparam;
			RECT minimum = { 0, 0, 8, 16 };
			DWORD style = (DWORD)GetWindowLongPtrA(window, GWL_STYLE);
			DWORD extended_style =
				(DWORD)GetWindowLongPtrA(window, GWL_EXSTYLE);

			/* Every built-in graphical font fits an 8 by 16 client area. */
			if (AdjustWindowRectEx(&minimum, style, FALSE, extended_style)) {
				limits->ptMinTrackSize.x = minimum.right - minimum.left;
				limits->ptMinTrackSize.y = minimum.bottom - minimum.top;
			}
			return 0;
		}
		else if (!(platform->flags & (FB_GFX3_WINDOW_FULLSCREEN |
		          FB_GFX3_WINDOW_NO_FRAME))) {
			MINMAXINFO *limits = (MINMAXINFO *)lparam;
			RECT minimum = { 0, 0, (LONG)platform->logical_width,
				(LONG)platform->logical_height };
			DWORD style = (DWORD)GetWindowLongPtrA(window, GWL_STYLE);
			DWORD extended_style =
				(DWORD)GetWindowLongPtrA(window, GWL_EXSTYLE);

			/*
				The ordinary border is fixed, but the maximize button may grant
				a larger client area. This matches gfxlib2's scaled-window
				distinction from GFX_RESIZABLE.
			*/
			if (AdjustWindowRectEx(&minimum, style, FALSE, extended_style)) {
				limits->ptMinTrackSize.x = minimum.right - minimum.left;
				limits->ptMinTrackSize.y = minimum.bottom - minimum.top;
			}
			return 0;
		}
		break;

	case WM_MOUSELEAVE:
		platform->mouse_tracking = FALSE;
		if (!platform_win32_mouse_in_client(window))
			fb_gfx3_input_platform_mouse_exit(platform->input);
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		button = BUTTON_LEFT;
		goto mouse_button_down;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		button = BUTTON_RIGHT;
		goto mouse_button_down;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		button = BUTTON_MIDDLE;
		goto mouse_button_down;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONDBLCLK:
		button = (HIWORD(wparam) == XBUTTON1) ? BUTTON_X1 : BUTTON_X2;
		goto mouse_button_down;
	case WM_LBUTTONUP:
		button = BUTTON_LEFT;
		goto mouse_button_up;
	case WM_RBUTTONUP:
		button = BUTTON_RIGHT;
		goto mouse_button_up;
	case WM_MBUTTONUP:
		button = BUTTON_MIDDLE;
		goto mouse_button_up;
	case WM_XBUTTONUP:
		button = (HIWORD(wparam) == XBUTTON1) ? BUTTON_X1 : BUTTON_X2;
		goto mouse_button_up;

	case WM_MOUSEWHEEL:
		fb_gfx3_input_platform_mouse_wheel(platform->input, FALSE,
			GET_WHEEL_DELTA_WPARAM(wparam));
		return 0;
	case WM_MOUSEHWHEEL:
		fb_gfx3_input_platform_mouse_wheel(platform->input, TRUE,
			GET_WHEEL_DELTA_WPARAM(wparam));
		return 0;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		{
			int translated_key;
			int ascii = platform_win32_key_ascii(wparam, lparam,
				&translated_key);
			int type = (lparam & 0x40000000) ? EVENT_KEY_REPEAT :
				EVENT_KEY_PRESS;

			fb_gfx3_input_platform_key(platform->input, type,
				fb_hVirtualToScancode((int)wparam), ascii);
			if (translated_key > 0xFF)
				fb_gfx3_input_platform_character(platform->input,
					translated_key, (uint32_t)LOWORD(lparam));
			if ((wparam == VK_F10) || (wparam == VK_MENU) ||
			    (translated_key == KEY_QUIT))
				return 0;
		}
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		fb_gfx3_input_platform_key(platform->input, EVENT_KEY_RELEASE,
			fb_hVirtualToScancode((int)wparam),
			platform_win32_key_ascii(wparam, lparam, NULL));
		if ((wparam == VK_F10) || (wparam == VK_MENU))
			return 0;
		break;

	case WM_CHAR:
		if (wparam < 256u) {
			int character = platform_win32_convert_character((int)wparam);

			fb_gfx3_input_platform_character(platform->input,
				character, (uint32_t)LOWORD(lparam));
		}
		return 0;

	case WM_CLOSE:
		if (!platform->close_requested) {
			platform->close_requested = TRUE;
			fb_gfx3_input_platform_close(platform->input);
		}
		ShowWindow(window, SW_HIDE);
		return 0;

	case WM_ERASEBKGND:
		return 1;

	case WM_SETCURSOR:
		if (LOWORD(lparam) == HTCLIENT) {
			SetCursor(platform->cursor_visible ?
				LoadCursorA(NULL, IDC_ARROW) : NULL);
			return TRUE;
		}
		break;

	default:
		break;
	}
	return DefWindowProcA(window, message, wparam, lparam);

mouse_button_up:
	platform->mouse_buttons &= ~button;
	fb_gfx3_input_platform_mouse_button(platform->input, button, FALSE,
		FALSE);
	if ((platform->mouse_buttons == 0) && (GetCapture() == window))
		ReleaseCapture();
	return 0;

mouse_button_down:
	/* A client button message itself proves that the pointer is inside. */
	platform_win32_track_mouse(platform, window, lparam);
	platform->mouse_buttons |= button;
	SetCapture(window);
	fb_gfx3_input_platform_mouse_button(platform->input, button, TRUE,
		(message == WM_LBUTTONDBLCLK) ||
		(message == WM_RBUTTONDBLCLK) ||
		(message == WM_MBUTTONDBLCLK) ||
		(message == WM_XBUTTONDBLCLK));
	return 0;
}

static int platform_win32_register_class(void)
{
	WNDCLASSA window_class;
	ATOM atom;

	memset(&window_class, 0, sizeof(window_class));
	window_class.style = CS_OWNDC | CS_DBLCLKS;
	window_class.lpfnWndProc = platform_win32_window_proc;
	window_class.hInstance = GetModuleHandleA(NULL);
	window_class.lpszClassName = FB_GFX3_WIN32_WINDOW_CLASS;
	atom = RegisterClassA(&window_class);
	if ((atom == 0) && (GetLastError() != ERROR_CLASS_ALREADY_EXISTS))
		return FB_GFX3_FAILED;
	return FB_GFX3_OK;
}

static int platform_win32_load_library_function(HMODULE library,
	const char *name, void *destination, size_t destination_size)
{
	FARPROC proc;

	if ((library == NULL) || (name == NULL) || (destination == NULL))
		return FB_GFX3_INVALID;
	proc = GetProcAddress(library, name);
	if ((proc == NULL) || (destination_size != sizeof(proc)))
		return FB_GFX3_UNSUPPORTED;
	memcpy(destination, (const void *)&proc, destination_size);
	return FB_GFX3_OK;
}

static int platform_win32_probe_opengl(void)
{
	HMODULE library = LoadLibraryA("opengl32.dll");

	if (library == NULL)
		return FB_GFX3_UNSUPPORTED;
	FreeLibrary(library);
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Optional XInput controller bridge                                         */
/* ------------------------------------------------------------------------- */

static float platform_win32_normalize_axis(SHORT value)
{
	if (value <= -32768)
		return -1.0f;
	if (value >= 32767)
		return 1.0f;
	return (float)value / 32767.0f;
}

static ssize_t platform_win32_xinput_buttons(
	const FB_GFX3_XINPUT_GAMEPAD *gamepad)
{
	ssize_t buttons = 0;

	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_A)
		buttons |= XPAD_BUTTON_A;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_B)
		buttons |= XPAD_BUTTON_B;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_X)
		buttons |= XPAD_BUTTON_X;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_Y)
		buttons |= XPAD_BUTTON_Y;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_LEFT_SHOULDER)
		buttons |= XPAD_BUTTON_L1;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_RIGHT_SHOULDER)
		buttons |= XPAD_BUTTON_R1;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_LEFT_THUMB)
		buttons |= XPAD_BUTTON_L3;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_RIGHT_THUMB)
		buttons |= XPAD_BUTTON_R3;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_START)
		buttons |= XPAD_BUTTON_START;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_BACK)
		buttons |= XPAD_BUTTON_SELECT;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_GUIDE)
		buttons |= XPAD_BUTTON_GUIDE;
	if (gamepad->left_trigger > 30u)
		buttons |= XPAD_BUTTON_L2;
	if (gamepad->right_trigger > 30u)
		buttons |= XPAD_BUTTON_R2;
	return buttons;
}

static ssize_t platform_win32_xinput_dpad(
	const FB_GFX3_XINPUT_GAMEPAD *gamepad)
{
	ssize_t dpad = 0;

	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_DPAD_UP)
		dpad |= XPAD_DPAD_UP;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_DPAD_RIGHT)
		dpad |= XPAD_DPAD_RIGHT;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_DPAD_DOWN)
		dpad |= XPAD_DPAD_DOWN;
	if (gamepad->buttons & FB_GFX3_XINPUT_GAMEPAD_DPAD_LEFT)
		dpad |= XPAD_DPAD_LEFT;
	return dpad;
}

static void platform_win32_load_xinput(FB_GFX3_PLATFORM_WIN32 *platform)
{
	static const char *const library_names[] = {
		"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll",
		"xinput1_2.dll", "xinput1_1.dll", NULL
	};
	FARPROC procedure;
	size_t i;

	if ((platform == NULL) || platform->xinput_load_attempted)
		return;
	platform->xinput_load_attempted = TRUE;
	for (i = 0; library_names[i] != NULL; ++i) {
		platform->xinput_library = LoadLibraryA(library_names[i]);
		if (platform->xinput_library == NULL)
			continue;
		procedure = GetProcAddress(platform->xinput_library,
			"XInputGetState");
		if ((procedure != NULL) &&
		    (sizeof(platform->xinput_get_state) == sizeof(procedure))) {
			memcpy((void *)&platform->xinput_get_state,
				(const void *)&procedure,
				sizeof(platform->xinput_get_state));
			return;
		}
		FreeLibrary(platform->xinput_library);
		platform->xinput_library = NULL;
	}
}

static void platform_win32_poll_xinput(FB_GFX3_PLATFORM_WIN32 *platform)
{
	FB_GFX3_XINPUT_STATE state;
	float axis[FB_GFX3_INPUT_GAMEPAD_AXIS_COUNT];
	ULONGLONG now;
	DWORD result;
	DWORD id;

	if ((platform == NULL) || (platform->input == NULL))
		return;
	now = GetTickCount64();
	if (now < platform->next_xinput_poll)
		return;
	platform->next_xinput_poll = now + FB_GFX3_XINPUT_POLL_MILLISECONDS;
	platform_win32_load_xinput(platform);
	if (platform->xinput_get_state == NULL)
		return;
	for (id = 0; id < 4; ++id) {
		memset(&state, 0, sizeof(state));
		result = platform->xinput_get_state(id, &state);
		if (result != ERROR_SUCCESS) {
			fb_gfx3_input_platform_gamepad_replace(platform->input,
				(int)id, FALSE, 0, NULL, 0.0f, 0.0f, 0);
			continue;
		}
		memset(axis, 0, sizeof(axis));
		axis[0] = platform_win32_normalize_axis(state.gamepad.left_x);
		axis[1] = platform_win32_normalize_axis(state.gamepad.left_y);
		axis[2] = platform_win32_normalize_axis(state.gamepad.right_x);
		axis[3] = platform_win32_normalize_axis(state.gamepad.right_y);
		fb_gfx3_input_platform_gamepad_replace(platform->input, (int)id,
			TRUE, platform_win32_xinput_buttons(&state.gamepad), axis,
			(float)state.gamepad.left_trigger / 255.0f,
			(float)state.gamepad.right_trigger / 255.0f,
			platform_win32_xinput_dpad(&state.gamepad));
	}
}

static void platform_win32_destroy(void *state)
{
	FB_GFX3_PLATFORM_WIN32 *platform = (FB_GFX3_PLATFORM_WIN32 *)state;

	if (platform == NULL)
		return;
	if (platform->mouse_clip)
		ClipCursor(NULL);
	if (platform->input != NULL)
		fb_gfx3_input_platform_window_info(platform->input, 0, 0,
			0, 0, 0, 0);
	if ((platform->wgl_make_current != NULL) &&
	    (platform->context != NULL))
		platform->wgl_make_current(NULL, NULL);
	if ((platform->wgl_delete_context != NULL) &&
	    (platform->context != NULL))
		platform->wgl_delete_context(platform->context);
	if ((platform->device_context != NULL) && (platform->window != NULL))
		ReleaseDC(platform->window, platform->device_context);
	if (platform->window != NULL)
		DestroyWindow(platform->window);
	if (platform->library != NULL)
		FreeLibrary(platform->library);
	if (platform->xinput_library != NULL)
		FreeLibrary(platform->xinput_library);
	free(platform);
}

static int platform_win32_create_window_base(void **destination, void *input,
	uint32_t width, uint32_t height, uint32_t flags, const char *title)
{
	FB_GFX3_PLATFORM_WIN32 *platform;
	MONITORINFO monitor_info;
	RECT window_rect;
	DWORD style;
	int window_x;
	int window_y;
	int window_width;
	int window_height;
	int result = FB_GFX3_FAILED;

	if ((destination == NULL) || (width == 0) || (height == 0) ||
	    (width > INT_MAX) || (height > INT_MAX))
		return FB_GFX3_INVALID;
	*destination = NULL;
	platform = (FB_GFX3_PLATFORM_WIN32 *)calloc(1, sizeof(*platform));
	if (platform == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	platform->input = (FB_GFX3_INPUT_STATE *)input;
	platform->cursor_visible = TRUE;
	platform->flags = flags;
	platform->logical_width = width;
	platform->logical_height = height;
	if (platform_win32_register_class() != FB_GFX3_OK)
		goto fail;

	style = WS_OVERLAPPEDWINDOW;
	if ((flags & FB_GFX3_WINDOW_RESIZABLE) == 0u)
		style &= ~(DWORD)WS_THICKFRAME;
	window_x = CW_USEDEFAULT;
	window_y = CW_USEDEFAULT;
	if ((flags & FB_GFX3_WINDOW_FULLSCREEN) != 0u) {
		memset(&monitor_info, 0, sizeof(monitor_info));
		monitor_info.cbSize = sizeof(monitor_info);
		if (!GetMonitorInfoA(MonitorFromPoint((POINT){ 0, 0 },
			MONITOR_DEFAULTTOPRIMARY), &monitor_info))
			goto fail;
		style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		window_x = monitor_info.rcMonitor.left;
		window_y = monitor_info.rcMonitor.top;
		window_rect.left = 0;
		window_rect.top = 0;
		window_rect.right = monitor_info.rcMonitor.right -
			monitor_info.rcMonitor.left;
		window_rect.bottom = monitor_info.rcMonitor.bottom -
			monitor_info.rcMonitor.top;
	} else {
		window_rect.left = 0;
		window_rect.top = 0;
		window_rect.right = (LONG)width;
		window_rect.bottom = (LONG)height;
		if ((flags & FB_GFX3_WINDOW_NO_FRAME) != 0u)
			style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		else if (!AdjustWindowRect(&window_rect, style, FALSE))
			goto fail;
	}
	if (((int64_t)window_rect.right - window_rect.left > INT_MAX) ||
	    ((int64_t)window_rect.bottom - window_rect.top > INT_MAX)) {
		result = FB_GFX3_INVALID;
		goto fail;
	}
	window_width = window_rect.right - window_rect.left;
	window_height = window_rect.bottom - window_rect.top;
	platform->window = CreateWindowExA(0, FB_GFX3_WIN32_WINDOW_CLASS,
		(title != NULL) ? title : "FreeBASIC gfxlib3",
		style, window_x, window_y, window_width,
		window_height, NULL, NULL, GetModuleHandleA(NULL), NULL);
	if (platform->window == NULL)
		goto fail;
	SetWindowLongPtrA(platform->window, GWLP_USERDATA, (LONG_PTR)platform);
	if (GetWindowRect(platform->window, &window_rect)) {
		fb_gfx3_input_platform_window_info(platform->input,
			(uintptr_t)platform->window, 0, window_rect.left,
			window_rect.top, GetSystemMetrics(SM_CXSCREEN),
			GetSystemMetrics(SM_CYSCREEN));
	}
	*destination = platform;
	return FB_GFX3_OK;

fail:
	platform_win32_destroy(platform);
	return result;
}

static int platform_win32_create_window(void **destination,
	const FB_GFX3_PLATFORM_WINDOW_CONFIG *config)
{
	if (config == NULL)
		return FB_GFX3_INVALID;
	return platform_win32_create_window_base(destination, config->input,
		config->width, config->height, config->flags, config->title);
}

static int platform_win32_create_opengl(void **destination,
	const FB_GFX3_PLATFORM_OPENGL_CONFIG *config)
{
	FB_GFX3_PLATFORM_WIN32 *platform;
	PIXELFORMATDESCRIPTOR format;
	HGLRC legacy_context = NULL;
	int attributes[9];
	int pixel_format;
	int result = FB_GFX3_FAILED;

	if ((destination == NULL) || (config == NULL) || (config->width == 0) ||
	    (config->height == 0) || (config->width > INT_MAX) ||
	    (config->height > INT_MAX) || (config->major_version > INT_MAX) ||
	    (config->minor_version > INT_MAX))
		return FB_GFX3_INVALID;
	result = platform_win32_create_window_base(destination, config->input,
		config->width, config->height, config->flags, config->title);
	if (result != FB_GFX3_OK)
		return result;
	platform = (FB_GFX3_PLATFORM_WIN32 *)*destination;
	*destination = NULL;
	platform->library = LoadLibraryA("opengl32.dll");
	if (platform->library == NULL) {
		result = FB_GFX3_UNSUPPORTED;
		goto fail;
	}
	if ((platform_win32_load_library_function(platform->library,
	     "wglCreateContext", (void *)&platform->wgl_create_context,
	     sizeof(platform->wgl_create_context)) != FB_GFX3_OK) ||
	    (platform_win32_load_library_function(platform->library,
	     "wglDeleteContext", (void *)&platform->wgl_delete_context,
	     sizeof(platform->wgl_delete_context)) != FB_GFX3_OK) ||
	    (platform_win32_load_library_function(platform->library,
	     "wglMakeCurrent", (void *)&platform->wgl_make_current,
	     sizeof(platform->wgl_make_current)) != FB_GFX3_OK) ||
	    (platform_win32_load_library_function(platform->library,
	     "wglGetProcAddress", (void *)&platform->wgl_get_proc_address,
	     sizeof(platform->wgl_get_proc_address)) != FB_GFX3_OK)) {
		result = FB_GFX3_UNSUPPORTED;
		goto fail;
	}
	platform->device_context = GetDC(platform->window);
	if (platform->device_context == NULL)
		goto fail;

	memset(&format, 0, sizeof(format));
	format.nSize = sizeof(format);
	format.nVersion = 1;
	format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER;
	format.iPixelType = PFD_TYPE_RGBA;
	format.cColorBits = 32;
	format.cAlphaBits = 8;
	format.iLayerType = PFD_MAIN_PLANE;
	pixel_format = ChoosePixelFormat(platform->device_context, &format);
	if ((pixel_format == 0) ||
	    !SetPixelFormat(platform->device_context, pixel_format, &format))
		goto fail;

	legacy_context = platform->wgl_create_context(platform->device_context);
	if (legacy_context == NULL) {
		result = FB_GFX3_UNSUPPORTED;
		goto fail;
	}
	if (!platform->wgl_make_current(platform->device_context,
	    legacy_context))
		goto fail;
	{
		PROC proc = platform->wgl_get_proc_address(
			"wglCreateContextAttribsARB");

		if ((proc == NULL) ||
		    (sizeof(platform->wgl_create_context_attributes) !=
		    sizeof(proc))) {
			result = FB_GFX3_UNSUPPORTED;
			goto fail;
		}
		memcpy((void *)&platform->wgl_create_context_attributes,
			(const void *)&proc,
			sizeof(platform->wgl_create_context_attributes));
	}
	{
		PROC proc = platform->wgl_get_proc_address("wglSwapIntervalEXT");

		if ((proc != NULL) &&
		    (sizeof(platform->wgl_swap_interval) == sizeof(proc)))
			memcpy((void *)&platform->wgl_swap_interval,
				(const void *)&proc,
				sizeof(platform->wgl_swap_interval));
	}
	attributes[0] = WGL_CONTEXT_MAJOR_VERSION_ARB;
	attributes[1] = (int)config->major_version;
	attributes[2] = WGL_CONTEXT_MINOR_VERSION_ARB;
	attributes[3] = (int)config->minor_version;
	attributes[4] = WGL_CONTEXT_PROFILE_MASK_ARB;
	attributes[5] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
	attributes[6] = 0;
	attributes[7] = 0;
	attributes[8] = 0;
	platform->context = platform->wgl_create_context_attributes(
		platform->device_context, NULL, attributes);
	platform->wgl_make_current(NULL, NULL);
	platform->wgl_delete_context(legacy_context);
	legacy_context = NULL;
	if (platform->context == NULL) {
		result = FB_GFX3_UNSUPPORTED;
		goto fail;
	}
	if (!platform->wgl_make_current(platform->device_context,
	    platform->context))
		goto fail;
	/*
		Page copies and SCREENUPDATE are explicit presentation boundaries.  A
		driver-selected swap interval would otherwise throttle every compatible
		frame to the monitor refresh rate and hide the GPU command throughput that
		gfxlib3 is designed to expose.  Programs that need a refresh boundary use
		SCREENSYNC, which remains an ordered renderer completion point.
	*/
	if (platform->wgl_swap_interval != NULL)
		platform->wgl_swap_interval(0);
	*destination = platform;
	return FB_GFX3_OK;

fail:
	if (legacy_context != NULL) {
		platform->wgl_make_current(NULL, NULL);
		platform->wgl_delete_context(legacy_context);
	}
	platform_win32_destroy(platform);
	return result;
}

static int platform_win32_native_handles(void *state, uintptr_t *instance,
	uintptr_t *window)
{
	FB_GFX3_PLATFORM_WIN32 *platform = (FB_GFX3_PLATFORM_WIN32 *)state;
	HMODULE module;

	if ((platform == NULL) || (instance == NULL) || (window == NULL))
		return FB_GFX3_INVALID;
	module = GetModuleHandleA(NULL);
	if ((module == NULL) || (platform->window == NULL))
		return FB_GFX3_FAILED;
	*instance = (uintptr_t)module;
	*window = (uintptr_t)platform->window;
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Render-thread window operations                                           */
/* ------------------------------------------------------------------------- */

static int platform_win32_load_opengl_function(void *state, const char *name,
	void *destination, size_t destination_size)
{
	FB_GFX3_PLATFORM_WIN32 *platform = (FB_GFX3_PLATFORM_WIN32 *)state;
	PROC proc;

	if ((platform == NULL) || (name == NULL) || (destination == NULL))
		return FB_GFX3_INVALID;
	proc = platform->wgl_get_proc_address(name);
	if ((proc == NULL) || (proc == (PROC)1) || (proc == (PROC)2) ||
	    (proc == (PROC)3) || (proc == (PROC)-1))
		proc = GetProcAddress(platform->library, name);
	if ((proc == NULL) || (destination_size != sizeof(proc)))
		return FB_GFX3_UNSUPPORTED;
	memcpy(destination, (const void *)&proc, destination_size);
	return FB_GFX3_OK;
}

static int platform_win32_client_size(void *state, uint32_t *width,
	uint32_t *height)
{
	FB_GFX3_PLATFORM_WIN32 *platform = (FB_GFX3_PLATFORM_WIN32 *)state;
	RECT client;
	LONG client_width;
	LONG client_height;

	if ((platform == NULL) || (width == NULL) || (height == NULL))
		return FB_GFX3_INVALID;
	if (!GetClientRect(platform->window, &client))
		return FB_GFX3_FAILED;
	client_width = client.right - client.left;
	client_height = client.bottom - client.top;
	if ((client_width < 0) || (client_height < 0))
		return FB_GFX3_FAILED;
	*width = (uint32_t)client_width;
	*height = (uint32_t)client_height;
	return FB_GFX3_OK;
}

/*
	This deliberately queries the desktop rather than any gfxlib3 window. It is
	used by SCREENINFO before a mode exists, where gfxlib2 reports the current
	user desktop but has no framebuffer pitch or bytes-per-pixel value yet.
*/
static int platform_win32_desktop_info(ssize_t *width, ssize_t *height,
	ssize_t *depth, ssize_t *refresh)
{
	DEVMODEA mode;

	memset(&mode, 0, sizeof(mode));
	mode.dmSize = sizeof(mode);
	if (!EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &mode))
		return FB_GFX3_FAILED;
	if ((mode.dmPelsWidth == 0) || (mode.dmPelsHeight == 0))
		return FB_GFX3_FAILED;
	if (width != NULL)
		*width = (ssize_t)mode.dmPelsWidth;
	if (height != NULL)
		*height = (ssize_t)mode.dmPelsHeight;
	if (depth != NULL)
		*depth = (ssize_t)mode.dmBitsPerPel;
	if (refresh != NULL)
		*refresh = (ssize_t)mode.dmDisplayFrequency;
	return FB_GFX3_OK;
}

static int platform_win32_swap_buffers(void *state)
{
	FB_GFX3_PLATFORM_WIN32 *platform = (FB_GFX3_PLATFORM_WIN32 *)state;

	if (platform == NULL)
		return FB_GFX3_INVALID;
	return SwapBuffers(platform->device_context) ? FB_GFX3_OK :
		FB_GFX3_FAILED;
}

static void platform_win32_pump_events(void *state)
{
	FB_GFX3_PLATFORM_WIN32 *platform = (FB_GFX3_PLATFORM_WIN32 *)state;
	MSG message;

	if (platform == NULL)
		return;
	platform_win32_apply_window_request(platform);
	platform_win32_apply_mouse_request(platform);
	platform_win32_poll_xinput(platform);
	while (PeekMessageA(&message, platform->window, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
}

static int platform_win32_show_window(void *state)
{
	FB_GFX3_PLATFORM_WIN32 *platform = (FB_GFX3_PLATFORM_WIN32 *)state;

	if (platform == NULL)
		return FB_GFX3_INVALID;
	if (!platform->shown) {
		ShowWindow(platform->window, SW_SHOW);
		/* A zero UpdateWindow result also means there was no paint region. */
		UpdateWindow(platform->window);
		platform->shown = TRUE;
	}
	return FB_GFX3_OK;
}

static int platform_win32_set_window_title(void *state, const char *title)
{
	FB_GFX3_PLATFORM_WIN32 *platform = (FB_GFX3_PLATFORM_WIN32 *)state;

	if ((platform == NULL) || (title == NULL))
		return FB_GFX3_INVALID;
	return SetWindowTextA(platform->window, title) ? FB_GFX3_OK :
		FB_GFX3_FAILED;
}

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_win32 = {
	"Win32",
	platform_win32_probe_opengl,
	platform_win32_create_window,
	platform_win32_create_opengl,
	platform_win32_native_handles,
	platform_win32_destroy,
	platform_win32_load_opengl_function,
	platform_win32_client_size,
	platform_win32_desktop_info,
	platform_win32_swap_buffers,
	platform_win32_pump_events,
	platform_win32_show_window,
	platform_win32_set_window_title
};

#else

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_win32 = {
	"Win32 unavailable",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

#endif

int fb_gfx3_platform_keyboard_overlay(void *platform,
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY *overlay)
{
	(void)platform;
	if (overlay != NULL)
		memset(overlay, 0, sizeof(*overlay));
	return FB_GFX3_UNSUPPORTED;
}

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_default(void)
{
	return &__fb_gfx3_platform_win32;
}

/* end of win32/gfx3_platform.c */
