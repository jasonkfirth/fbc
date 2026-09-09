/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: darwin/gfx3_platform.c

    Purpose:

        Own the Cocoa window and CAMetalLayer used by the macOS Vulkan backend.

    Responsibilities:

        - create a native AppKit window with a Metal-compatible backing layer
        - expose that layer to MoltenVK's VK_EXT_metal_surface entry point
        - translate Cocoa keyboard, mouse, focus, close, move, and resize events
        - apply synchronized cursor and window requests on the owning thread

    This file intentionally does NOT contain:

        - Vulkan calls, shaders, or GPU resource management
        - software framebuffer presentation
        - OpenGL context creation
*/

#include "../gfx3_platform.h"
#include "../gfx3_input.h"

#if defined(HOST_DARWIN)

#include <CoreGraphics/CGDirectDisplay.h>
#include <CoreGraphics/CGEvent.h>
#include <CoreGraphics/CGGeometry.h>
#include <limits.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>

#define FB_GFX3_DARWIN_STYLE_TITLED       1UL
#define FB_GFX3_DARWIN_STYLE_CLOSABLE     2UL
#define FB_GFX3_DARWIN_STYLE_MINIATURIZABLE 4UL
#define FB_GFX3_DARWIN_STYLE_RESIZABLE    8UL
#define FB_GFX3_DARWIN_BACKING_BUFFERED   2UL
#define FB_GFX3_DARWIN_ACTIVATION_REGULAR 0L
#define FB_GFX3_DARWIN_VIEW_WIDTH_SIZABLE 2UL
#define FB_GFX3_DARWIN_VIEW_HEIGHT_SIZABLE 16UL

#define FB_GFX3_DARWIN_EVENT_LEFT_MOUSE_DOWN      1UL
#define FB_GFX3_DARWIN_EVENT_LEFT_MOUSE_UP        2UL
#define FB_GFX3_DARWIN_EVENT_RIGHT_MOUSE_DOWN     3UL
#define FB_GFX3_DARWIN_EVENT_RIGHT_MOUSE_UP       4UL
#define FB_GFX3_DARWIN_EVENT_MOUSE_MOVED          5UL
#define FB_GFX3_DARWIN_EVENT_LEFT_MOUSE_DRAGGED   6UL
#define FB_GFX3_DARWIN_EVENT_RIGHT_MOUSE_DRAGGED  7UL
#define FB_GFX3_DARWIN_EVENT_MOUSE_ENTERED        8UL
#define FB_GFX3_DARWIN_EVENT_MOUSE_EXITED         9UL
#define FB_GFX3_DARWIN_EVENT_KEY_DOWN            10UL
#define FB_GFX3_DARWIN_EVENT_KEY_UP              11UL
#define FB_GFX3_DARWIN_EVENT_FLAGS_CHANGED       12UL
#define FB_GFX3_DARWIN_EVENT_SCROLL_WHEEL        22UL
#define FB_GFX3_DARWIN_EVENT_OTHER_MOUSE_DOWN    25UL
#define FB_GFX3_DARWIN_EVENT_OTHER_MOUSE_UP      26UL
#define FB_GFX3_DARWIN_EVENT_OTHER_MOUSE_DRAGGED 27UL

#define FB_GFX3_DARWIN_MOD_CAPSLOCK (1UL << 16)
#define FB_GFX3_DARWIN_MOD_SHIFT    (1UL << 17)
#define FB_GFX3_DARWIN_MOD_CONTROL  (1UL << 18)
#define FB_GFX3_DARWIN_MOD_OPTION   (1UL << 19)
#define FB_GFX3_DARWIN_MOD_COMMAND  (1UL << 20)

typedef struct FB_GFX3_PLATFORM_DARWIN {
	id pool;
	id app;
	id window;
	id view;
	id layer;
	id run_loop_mode;
	FB_GFX3_INPUT_STATE *input;
	uint32_t flags;
	uint32_t logical_width;
	uint32_t logical_height;
	uint32_t view_width;
	uint32_t view_height;
	int window_x;
	int window_y;
	int focused;
	int shown;
	int close_requested;
	int cursor_visible;
	int mouse_clip;
} FB_GFX3_PLATFORM_DARWIN;

static Class platform_darwin_view_class = Nil;

/* ------------------------------------------------------------------------- */
/* Objective-C runtime calls                                                 */
/* ------------------------------------------------------------------------- */

static SEL platform_darwin_selector(const char *name)
{
	return sel_registerName(name);
}

static id platform_darwin_id(id object, const char *name)
{
	return ((id (*)(id, SEL))objc_msgSend)(object,
		platform_darwin_selector(name));
}

static id platform_darwin_id_cstring(id object, const char *name,
	const char *value)
{
	return ((id (*)(id, SEL, const char *))objc_msgSend)(object,
		platform_darwin_selector(name), value);
}

static id platform_darwin_id_rect(id object, const char *name, CGRect value)
{
	return ((id (*)(id, SEL, CGRect))objc_msgSend)(object,
		platform_darwin_selector(name), value);
}

static id platform_darwin_id_rect_ulong_ulong_bool(id object,
	const char *name, CGRect rect, unsigned long style,
	unsigned long backing, BOOL defer_window)
{
	return ((id (*)(id, SEL, CGRect, unsigned long, unsigned long, BOOL))
		objc_msgSend)(object, platform_darwin_selector(name), rect, style,
		backing, defer_window);
}

static id platform_darwin_next_event(id object, unsigned long mask,
	id date, id mode, BOOL dequeue)
{
	return ((id (*)(id, SEL, unsigned long, id, id, BOOL))objc_msgSend)(
		object, platform_darwin_selector(
		"nextEventMatchingMask:untilDate:inMode:dequeue:"), mask, date,
		mode, dequeue);
}

static void platform_darwin_void(id object, const char *name)
{
	((void (*)(id, SEL))objc_msgSend)(object,
		platform_darwin_selector(name));
}

static void platform_darwin_void_id(id object, const char *name, id value)
{
	((void (*)(id, SEL, id))objc_msgSend)(object,
		platform_darwin_selector(name), value);
}

static void platform_darwin_void_bool(id object, const char *name, BOOL value)
{
	((void (*)(id, SEL, BOOL))objc_msgSend)(object,
		platform_darwin_selector(name), value);
}

static void platform_darwin_void_long(id object, const char *name, long value)
{
	((void (*)(id, SEL, long))objc_msgSend)(object,
		platform_darwin_selector(name), value);
}

static void platform_darwin_void_ulong(id object, const char *name,
	unsigned long value)
{
	((void (*)(id, SEL, unsigned long))objc_msgSend)(object,
		platform_darwin_selector(name), value);
}

static void platform_darwin_void_point(id object, const char *name,
	CGPoint value)
{
	((void (*)(id, SEL, CGPoint))objc_msgSend)(object,
		platform_darwin_selector(name), value);
}

static void platform_darwin_void_size(id object, const char *name,
	CGSize value)
{
	((void (*)(id, SEL, CGSize))objc_msgSend)(object,
		platform_darwin_selector(name), value);
}

static BOOL platform_darwin_bool(id object, const char *name)
{
	return ((BOOL (*)(id, SEL))objc_msgSend)(object,
		platform_darwin_selector(name));
}

static BOOL platform_darwin_bool_id(id object, const char *name, id value)
{
	return ((BOOL (*)(id, SEL, id))objc_msgSend)(object,
		platform_darwin_selector(name), value);
}

static unsigned long platform_darwin_ulong(id object, const char *name)
{
	return ((unsigned long (*)(id, SEL))objc_msgSend)(object,
		platform_darwin_selector(name));
}

static unsigned short platform_darwin_ushort_ulong(id object,
	const char *name, unsigned long index)
{
	return ((unsigned short (*)(id, SEL, unsigned long))objc_msgSend)(object,
		platform_darwin_selector(name), index);
}

static double platform_darwin_double(id object, const char *name)
{
	return ((double (*)(id, SEL))objc_msgSend)(object,
		platform_darwin_selector(name));
}

static CGPoint platform_darwin_point(id object, const char *name)
{
	return ((CGPoint (*)(id, SEL))objc_msgSend)(object,
		platform_darwin_selector(name));
}

static CGPoint platform_darwin_point_point(id object, const char *name,
	CGPoint point)
{
	return ((CGPoint (*)(id, SEL, CGPoint))objc_msgSend)(object,
		platform_darwin_selector(name), point);
}

static CGPoint platform_darwin_point_point_id(id object, const char *name,
	CGPoint point, id view)
{
	return ((CGPoint (*)(id, SEL, CGPoint, id))objc_msgSend)(object,
		platform_darwin_selector(name), point, view);
}

static CGRect platform_darwin_rect(id object, const char *name)
{
	CGRect rect;

#if defined(__x86_64__) && !defined(__arm64__)
	((void (*)(CGRect *, id, SEL))objc_msgSend_stret)(&rect, object,
		platform_darwin_selector(name));
#else
	rect = ((CGRect (*)(id, SEL))objc_msgSend)(object,
		platform_darwin_selector(name));
#endif
	return rect;
}

static id platform_darwin_string(const char *text)
{
	Class string_class = objc_getClass("NSString");

	if (text == NULL)
		text = "FreeBASIC gfxlib3";
	if (string_class == Nil)
		return nil;
	return platform_darwin_id_cstring((id)string_class,
		"stringWithUTF8String:", text);
}

/* ------------------------------------------------------------------------- */
/* Native view and coordinate handling                                       */
/* ------------------------------------------------------------------------- */

static BOOL platform_darwin_view_is_flipped(id self, SEL command)
{
	(void)self;
	(void)command;
	return YES;
}

static BOOL platform_darwin_view_accepts_first_responder(id self, SEL command)
{
	(void)self;
	(void)command;
	return YES;
}

static void platform_darwin_view_did_move_to_window(id self, SEL command)
{
	id window;

	(void)command;
	window = platform_darwin_id(self, "window");
	if (window == nil)
		return;
	platform_darwin_void_bool(window, "setAcceptsMouseMovedEvents:", YES);
	platform_darwin_bool_id(window, "makeFirstResponder:", self);
}

static int platform_darwin_ensure_view_class(void)
{
	Class base_class;

	if (platform_darwin_view_class != Nil)
		return TRUE;
	base_class = objc_getClass("NSView");
	if (base_class == Nil)
		return FALSE;
	platform_darwin_view_class = objc_allocateClassPair(base_class,
		"FBGfx3MetalView", 0);
	if (platform_darwin_view_class == Nil)
		return FALSE;
	if (!class_addMethod(platform_darwin_view_class,
	    platform_darwin_selector("isFlipped"),
	    (IMP)platform_darwin_view_is_flipped, "B@:") ||
	    !class_addMethod(platform_darwin_view_class,
	    platform_darwin_selector("acceptsFirstResponder"),
	    (IMP)platform_darwin_view_accepts_first_responder, "B@:") ||
	    !class_addMethod(platform_darwin_view_class,
	    platform_darwin_selector("viewDidMoveToWindow"),
	    (IMP)platform_darwin_view_did_move_to_window, "v@:")) {
		objc_disposeClassPair(platform_darwin_view_class);
		platform_darwin_view_class = Nil;
		return FALSE;
	}
	objc_registerClassPair(platform_darwin_view_class);
	return TRUE;
}

static void platform_darwin_refresh_geometry(
	FB_GFX3_PLATFORM_DARWIN *platform)
{
	CGRect bounds;
	CGRect frame;
	double scale;
	CGSize drawable_size;

	if ((platform == NULL) || (platform->view == nil))
		return;
	bounds = platform_darwin_rect(platform->view, "bounds");
	if ((bounds.size.width > 0.0) && (bounds.size.height > 0.0)) {
		platform->view_width = (uint32_t)bounds.size.width;
		platform->view_height = (uint32_t)bounds.size.height;
	}
	if (platform->window != nil) {
		frame = platform_darwin_rect(platform->window, "frame");
		platform->window_x = (int)frame.origin.x;
		platform->window_y = (int)frame.origin.y;
	}
	if (platform->layer != nil) {
		scale = platform_darwin_double(platform->window,
			"backingScaleFactor");
		if (scale <= 0.0)
			scale = 1.0;
		drawable_size.width = bounds.size.width * scale;
		drawable_size.height = bounds.size.height * scale;
		platform_darwin_void_size(platform->layer, "setDrawableSize:",
			drawable_size);
		((void (*)(id, SEL, double))objc_msgSend)(platform->layer,
			platform_darwin_selector("setContentsScale:"), scale);
	}
}

static void platform_darwin_client_to_logical(
	FB_GFX3_PLATFORM_DARWIN *platform, int client_x, int client_y,
	int *logical_x, int *logical_y)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;

	if ((platform == NULL) ||
	    (platform->flags & FB_GFX3_WINDOW_RESIZABLE) ||
	    (fb_gfx3_platform_presentation_layout(platform->logical_width,
	    platform->logical_height, platform->view_width,
	    platform->view_height, &layout) != FB_GFX3_OK)) {
		if (logical_x != NULL)
			*logical_x = client_x;
		if (logical_y != NULL)
			*logical_y = client_y;
		return;
	}
	fb_gfx3_platform_client_to_logical(&layout, platform->logical_width,
		platform->logical_height, client_x, client_y, logical_x, logical_y);
}

static void platform_darwin_logical_to_client(
	FB_GFX3_PLATFORM_DARWIN *platform, int logical_x, int logical_y,
	int *client_x, int *client_y)
{
	FB_GFX3_PRESENTATION_LAYOUT layout;

	if ((platform == NULL) ||
	    (platform->flags & FB_GFX3_WINDOW_RESIZABLE) ||
	    (fb_gfx3_platform_presentation_layout(platform->logical_width,
	    platform->logical_height, platform->view_width,
	    platform->view_height, &layout) != FB_GFX3_OK)) {
		if (client_x != NULL)
			*client_x = logical_x;
		if (client_y != NULL)
			*client_y = logical_y;
		return;
	}
	fb_gfx3_platform_logical_to_client(&layout, platform->logical_width,
		platform->logical_height, logical_x, logical_y, client_x, client_y);
}

static void platform_darwin_event_position(FB_GFX3_PLATFORM_DARWIN *platform,
	id event, int *logical_x, int *logical_y)
{
	CGPoint point;

	point = platform_darwin_point(event, "locationInWindow");
	point = platform_darwin_point_point_id(platform->view,
		"convertPoint:fromView:", point, nil);
	platform_darwin_client_to_logical(platform, (int)point.x, (int)point.y,
		logical_x, logical_y);
}

/* ------------------------------------------------------------------------- */
/* Keyboard translation                                                      */
/* ------------------------------------------------------------------------- */

static int platform_darwin_keycode_to_scancode(unsigned short keycode)
{
	switch (keycode) {
	case 0x00: return SC_A;
	case 0x01: return SC_S;
	case 0x02: return SC_D;
	case 0x03: return SC_F;
	case 0x04: return SC_H;
	case 0x05: return SC_G;
	case 0x06: return SC_Z;
	case 0x07: return SC_X;
	case 0x08: return SC_C;
	case 0x09: return SC_V;
	case 0x0B: return SC_B;
	case 0x0C: return SC_Q;
	case 0x0D: return SC_W;
	case 0x0E: return SC_E;
	case 0x0F: return SC_R;
	case 0x10: return SC_Y;
	case 0x11: return SC_T;
	case 0x12: return SC_1;
	case 0x13: return SC_2;
	case 0x14: return SC_3;
	case 0x15: return SC_4;
	case 0x16: return SC_6;
	case 0x17: return SC_5;
	case 0x18: return SC_EQUALS;
	case 0x19: return SC_9;
	case 0x1A: return SC_7;
	case 0x1B: return SC_MINUS;
	case 0x1C: return SC_8;
	case 0x1D: return SC_0;
	case 0x1E: return SC_RIGHTBRACKET;
	case 0x1F: return SC_O;
	case 0x20: return SC_U;
	case 0x21: return SC_LEFTBRACKET;
	case 0x22: return SC_I;
	case 0x23: return SC_P;
	case 0x24: return SC_ENTER;
	case 0x25: return SC_L;
	case 0x26: return SC_J;
	case 0x27: return SC_QUOTE;
	case 0x28: return SC_K;
	case 0x29: return SC_SEMICOLON;
	case 0x2A: return SC_BACKSLASH;
	case 0x2B: return SC_COMMA;
	case 0x2C: return SC_SLASH;
	case 0x2D: return SC_N;
	case 0x2E: return SC_M;
	case 0x2F: return SC_PERIOD;
	case 0x30: return SC_TAB;
	case 0x31: return SC_SPACE;
	case 0x32: return SC_TILDE;
	case 0x33: return SC_BACKSPACE;
	case 0x35: return SC_ESCAPE;
	case 0x37: return SC_LWIN;
	case 0x38: return SC_LSHIFT;
	case 0x39: return SC_CAPSLOCK;
	case 0x3A: return SC_ALT;
	case 0x3B: return SC_CONTROL;
	case 0x3C: return SC_RSHIFT;
	case 0x3D: return SC_ALTGR;
	case 0x3E: return SC_CONTROL;
	case 0x47: return SC_CLEAR;
	case 0x4B:
	case 0x4C: return SC_ENTER;
	case 0x4E: return SC_PLUS;
	case 0x51: return SC_0;
	case 0x52:
	case 0x72: return SC_INSERT;
	case 0x53:
	case 0x77: return SC_END;
	case 0x54:
	case 0x7D: return SC_DOWN;
	case 0x55:
	case 0x79: return SC_PAGEDOWN;
	case 0x56:
	case 0x7B: return SC_LEFT;
	case 0x57: return SC_CLEAR;
	case 0x58:
	case 0x7C: return SC_RIGHT;
	case 0x59:
	case 0x73: return SC_HOME;
	case 0x5A:
	case 0x7E: return SC_UP;
	case 0x5B:
	case 0x74: return SC_PAGEUP;
	case 0x60: return SC_F5;
	case 0x61: return SC_F6;
	case 0x62: return SC_F7;
	case 0x63: return SC_F3;
	case 0x64: return SC_F8;
	case 0x65: return SC_F9;
	case 0x67: return SC_F11;
	case 0x6D: return SC_F10;
	case 0x6F: return SC_F12;
	case 0x75: return SC_DELETE;
	case 0x76: return SC_F4;
	case 0x78: return SC_F2;
	case 0x7A: return SC_F1;
	default: return 0;
	}
}

static int platform_darwin_event_key(id event, int scancode)
{
	id characters;
	unsigned long length;
	unsigned short character;

	characters = platform_darwin_id(event, "characters");
	if (characters != nil) {
		length = platform_darwin_ulong(characters, "length");
		if (length == 1u) {
			character = platform_darwin_ushort_ulong(characters,
				"characterAtIndex:", 0u);
			if (character == 127u)
				return KEY_BACKSPACE;
			if ((character > 0u) && (character <= 0xFFu))
				return (int)character;
		}
	}
	return fb_hScancodeToExtendedKey(scancode);
}

static void platform_darwin_key_event(FB_GFX3_PLATFORM_DARWIN *platform,
	id event, int pressed)
{
	int scancode;
	int key;
	int type;
	unsigned short keycode;

	keycode = (unsigned short)platform_darwin_ulong(event, "keyCode");
	scancode = platform_darwin_keycode_to_scancode(keycode);
	key = platform_darwin_event_key(event, scancode);
	type = pressed ? (platform_darwin_bool(event, "isARepeat") ?
		EVENT_KEY_REPEAT : EVENT_KEY_PRESS) : EVENT_KEY_RELEASE;
	fb_gfx3_input_platform_key(platform->input, type, scancode,
		((key > 0) && (key <= 0xFF)) ? key : 0);
	if (pressed)
		fb_gfx3_input_platform_character(platform->input, key, 1u);
}

static void platform_darwin_modifier_event(
	FB_GFX3_PLATFORM_DARWIN *platform, id event)
{
	unsigned short keycode;
	unsigned long flags;
	int pressed;
	int scancode;

	keycode = (unsigned short)platform_darwin_ulong(event, "keyCode");
	flags = platform_darwin_ulong(event, "modifierFlags");
	scancode = platform_darwin_keycode_to_scancode(keycode);
	if (scancode == 0)
		return;
	switch (keycode) {
	case 0x38:
	case 0x3C:
		pressed = (flags & FB_GFX3_DARWIN_MOD_SHIFT) != 0u;
		break;
	case 0x3B:
	case 0x3E:
		pressed = (flags & FB_GFX3_DARWIN_MOD_CONTROL) != 0u;
		break;
	case 0x3A:
	case 0x3D:
		pressed = (flags & FB_GFX3_DARWIN_MOD_OPTION) != 0u;
		break;
	case 0x37:
		pressed = (flags & FB_GFX3_DARWIN_MOD_COMMAND) != 0u;
		break;
	case 0x39:
		pressed = (flags & FB_GFX3_DARWIN_MOD_CAPSLOCK) != 0u;
		break;
	default:
		return;
	}
	fb_gfx3_input_platform_key(platform->input,
		pressed ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE, scancode, 0);
}

/* ------------------------------------------------------------------------- */
/* Event pump and synchronized requests                                      */
/* ------------------------------------------------------------------------- */

static int platform_darwin_mouse_button(unsigned long button)
{
	switch (button) {
	case 0u: return BUTTON_LEFT;
	case 1u: return BUTTON_RIGHT;
	case 2u: return BUTTON_MIDDLE;
	case 3u: return BUTTON_X1;
	case 4u: return BUTTON_X2;
	default: return 0;
	}
}

static void platform_darwin_handle_event(
	FB_GFX3_PLATFORM_DARWIN *platform, id event)
{
	unsigned long type;
	int x;
	int y;
	int button;
	double delta;

	type = platform_darwin_ulong(event, "type");
	switch (type) {
	case FB_GFX3_DARWIN_EVENT_KEY_DOWN:
		platform_darwin_key_event(platform, event, TRUE);
		break;
	case FB_GFX3_DARWIN_EVENT_KEY_UP:
		platform_darwin_key_event(platform, event, FALSE);
		break;
	case FB_GFX3_DARWIN_EVENT_FLAGS_CHANGED:
		platform_darwin_modifier_event(platform, event);
		break;
	case FB_GFX3_DARWIN_EVENT_MOUSE_MOVED:
	case FB_GFX3_DARWIN_EVENT_LEFT_MOUSE_DRAGGED:
	case FB_GFX3_DARWIN_EVENT_RIGHT_MOUSE_DRAGGED:
	case FB_GFX3_DARWIN_EVENT_OTHER_MOUSE_DRAGGED:
		platform_darwin_event_position(platform, event, &x, &y);
		fb_gfx3_input_platform_mouse_enter(platform->input);
		fb_gfx3_input_platform_mouse_move(platform->input, x, y);
		break;
	case FB_GFX3_DARWIN_EVENT_MOUSE_ENTERED:
		fb_gfx3_input_platform_mouse_enter(platform->input);
		break;
	case FB_GFX3_DARWIN_EVENT_MOUSE_EXITED:
		fb_gfx3_input_platform_mouse_exit(platform->input);
		break;
	case FB_GFX3_DARWIN_EVENT_LEFT_MOUSE_DOWN:
	case FB_GFX3_DARWIN_EVENT_RIGHT_MOUSE_DOWN:
	case FB_GFX3_DARWIN_EVENT_OTHER_MOUSE_DOWN:
		platform_darwin_event_position(platform, event, &x, &y);
		fb_gfx3_input_platform_mouse_enter(platform->input);
		fb_gfx3_input_platform_mouse_move(platform->input, x, y);
		button = platform_darwin_mouse_button(
			platform_darwin_ulong(event, "buttonNumber"));
		if (button != 0)
			fb_gfx3_input_platform_mouse_button(platform->input, button,
				TRUE, platform_darwin_ulong(event, "clickCount") > 1u);
		break;
	case FB_GFX3_DARWIN_EVENT_LEFT_MOUSE_UP:
	case FB_GFX3_DARWIN_EVENT_RIGHT_MOUSE_UP:
	case FB_GFX3_DARWIN_EVENT_OTHER_MOUSE_UP:
		button = platform_darwin_mouse_button(
			platform_darwin_ulong(event, "buttonNumber"));
		if (button != 0)
			fb_gfx3_input_platform_mouse_button(platform->input, button,
				FALSE, FALSE);
		break;
	case FB_GFX3_DARWIN_EVENT_SCROLL_WHEEL:
		delta = platform_darwin_double(event, "deltaY");
		if (delta != 0.0)
			fb_gfx3_input_platform_mouse_wheel(platform->input, FALSE,
				delta > 0.0 ? 1 : -1);
		delta = platform_darwin_double(event, "deltaX");
		if (delta != 0.0)
			fb_gfx3_input_platform_mouse_wheel(platform->input, TRUE,
				delta > 0.0 ? 1 : -1);
		break;
	default:
		break;
	}
}

static void platform_darwin_apply_requests(
	FB_GFX3_PLATFORM_DARWIN *platform)
{
	FB_GFX3_MOUSE_REQUEST mouse;
	FB_GFX3_WINDOW_REQUEST window;
	CGPoint point;
	CGPoint window_point;
	CGPoint screen_point;
	CGRect display_bounds;
	int client_x;
	int client_y;

	if (fb_gfx3_input_platform_take_window_request(platform->input,
	    &window) && (window.flags & FB_GFX3_WINDOW_REQUEST_POSITION) &&
	    !(platform->flags & FB_GFX3_WINDOW_FULLSCREEN)) {
		point.x = (CGFloat)window.x;
		point.y = (CGFloat)window.y;
		platform_darwin_void_point(platform->window, "setFrameOrigin:", point);
		fb_gfx3_input_platform_window_moved(platform->input,
			window.x, window.y);
	}
	if (!fb_gfx3_input_platform_take_mouse_request(platform->input, &mouse))
		return;
	if (mouse.flags & FB_GFX3_MOUSE_REQUEST_POSITION) {
		platform_darwin_logical_to_client(platform, mouse.x, mouse.y,
			&client_x, &client_y);
		point.x = (CGFloat)client_x;
		point.y = (CGFloat)client_y;
		window_point = platform_darwin_point_point_id(platform->view,
			"convertPoint:toView:", point, nil);
		screen_point = platform_darwin_point_point(platform->window,
			"convertBaseToScreen:", window_point);
		display_bounds = CGDisplayBounds(CGMainDisplayID());
		point.x = screen_point.x;
		point.y = display_bounds.size.height - screen_point.y;
		CGWarpMouseCursorPosition(point);
	}
	if ((mouse.flags & FB_GFX3_MOUSE_REQUEST_CURSOR) &&
	    (platform->cursor_visible != (mouse.cursor != 0))) {
		if (mouse.cursor != 0)
			platform_darwin_void((id)objc_getClass("NSCursor"), "unhide");
		else
			platform_darwin_void((id)objc_getClass("NSCursor"), "hide");
		platform->cursor_visible = mouse.cursor != 0;
	}
	if (mouse.flags & FB_GFX3_MOUSE_REQUEST_CLIP) {
		/* AppKit has no supported window-local pointer confinement API. */
		platform->mouse_clip = mouse.clip != 0;
	}
}

static void platform_darwin_publish_window_info(
	FB_GFX3_PLATFORM_DARWIN *platform)
{
	ssize_t desktop_width = 0;
	ssize_t desktop_height = 0;

	platform_darwin_refresh_geometry(platform);
	desktop_width = (ssize_t)CGDisplayPixelsWide(CGMainDisplayID());
	desktop_height = (ssize_t)CGDisplayPixelsHigh(CGMainDisplayID());
	fb_gfx3_input_platform_window_info(platform->input,
		(uintptr_t)platform->window, (uintptr_t)CGMainDisplayID(),
		platform->window_x, platform->window_y, (int)desktop_width,
		(int)desktop_height);
}

static void platform_darwin_pump_events(void *state)
{
	FB_GFX3_PLATFORM_DARWIN *platform =
		(FB_GFX3_PLATFORM_DARWIN *)state;
	id event_pool;
	id event;
	uint32_t old_width;
	uint32_t old_height;
	int focused;

	if (platform == NULL)
		return;
	/*
		Cocoa event delivery creates autoreleased wrapper objects. A nested pool
		keeps a long-running graphics program from retaining one allocation for
		every input event until the window finally closes.
	*/
	event_pool = platform_darwin_id(
		(id)objc_getClass("NSAutoreleasePool"), "alloc");
	event_pool = platform_darwin_id(event_pool, "init");
	platform_darwin_apply_requests(platform);
	while ((event = platform_darwin_next_event(platform->app, ULONG_MAX,
	    nil, platform->run_loop_mode, YES)) != nil) {
		platform_darwin_handle_event(platform, event);
		platform_darwin_void_id(platform->app, "sendEvent:", event);
	}
	platform_darwin_void(platform->app, "updateWindows");
	old_width = platform->view_width;
	old_height = platform->view_height;
	platform_darwin_publish_window_info(platform);
	if ((platform->flags & FB_GFX3_WINDOW_RESIZABLE) &&
	    ((platform->view_width != old_width) ||
	    (platform->view_height != old_height)))
		fb_gfx3_input_platform_resize(platform->input, platform->view_width,
			platform->view_height);
	focused = platform_darwin_bool(platform->window, "isKeyWindow") != NO;
	if (focused != platform->focused) {
		platform->focused = focused;
		fb_gfx3_input_platform_focus(platform->input, focused);
	}
	if (platform->shown && !platform->close_requested &&
	    !platform_darwin_bool(platform->window, "isVisible")) {
		platform->close_requested = TRUE;
		fb_gfx3_input_platform_close(platform->input);
	}
	if (event_pool != nil)
		platform_darwin_void(event_pool, "release");
}

/* ------------------------------------------------------------------------- */
/* Window lifecycle and platform vtable                                      */
/* ------------------------------------------------------------------------- */

static int platform_darwin_create_window(void **destination,
	const FB_GFX3_PLATFORM_WINDOW_CONFIG *config)
{
	FB_GFX3_PLATFORM_DARWIN *platform;
	Class application_class;
	Class pool_class;
	Class window_class;
	Class layer_class;
	CGRect frame;
	CGRect view_frame;
	CGSize minimum_size;
	unsigned long style;

	if ((destination == NULL) || (config == NULL) ||
	    (config->width == 0u) || (config->height == 0u) ||
	    (config->width > INT_MAX) || (config->height > INT_MAX))
		return FB_GFX3_INVALID;
	*destination = NULL;
	application_class = objc_getClass("NSApplication");
	pool_class = objc_getClass("NSAutoreleasePool");
	window_class = objc_getClass("NSWindow");
	layer_class = objc_getClass("CAMetalLayer");
	if ((application_class == Nil) || (pool_class == Nil) ||
	    (window_class == Nil) || (layer_class == Nil) ||
	    !platform_darwin_ensure_view_class())
		return FB_GFX3_UNSUPPORTED;
	platform = (FB_GFX3_PLATFORM_DARWIN *)calloc(1, sizeof(*platform));
	if (platform == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	platform->pool = platform_darwin_id((id)pool_class, "alloc");
	platform->pool = platform_darwin_id(platform->pool, "init");
	if (platform->pool == nil)
		goto fail;
	platform->app = platform_darwin_id((id)application_class,
		"sharedApplication");
	if (platform->app == nil)
		goto fail;
	platform->run_loop_mode = platform_darwin_string(
		"kCFRunLoopDefaultMode");
	platform->input = (FB_GFX3_INPUT_STATE *)config->input;
	platform->flags = config->flags;
	platform->logical_width = config->width;
	platform->logical_height = config->height;
	platform->view_width = config->width;
	platform->view_height = config->height;
	platform->cursor_visible = TRUE;
	platform_darwin_void_long(platform->app, "setActivationPolicy:",
		FB_GFX3_DARWIN_ACTIVATION_REGULAR);
	platform_darwin_void(platform->app, "finishLaunching");
	if (config->flags & FB_GFX3_WINDOW_FULLSCREEN)
		frame = CGDisplayBounds(CGMainDisplayID());
	else
		frame = CGRectMake(0.0, 0.0, (CGFloat)config->width,
			(CGFloat)config->height);
	if ((frame.size.width <= 0.0) || (frame.size.height <= 0.0))
		goto fail;
	view_frame = CGRectMake(0.0, 0.0, frame.size.width, frame.size.height);
	if (config->flags &
	    (FB_GFX3_WINDOW_FULLSCREEN | FB_GFX3_WINDOW_NO_FRAME)) {
		style = 0u;
	} else {
		style = FB_GFX3_DARWIN_STYLE_TITLED |
			FB_GFX3_DARWIN_STYLE_CLOSABLE |
			FB_GFX3_DARWIN_STYLE_MINIATURIZABLE;
		if (config->flags & FB_GFX3_WINDOW_RESIZABLE)
			style |= FB_GFX3_DARWIN_STYLE_RESIZABLE;
	}
	platform->window = platform_darwin_id((id)window_class, "alloc");
	platform->window = platform_darwin_id_rect_ulong_ulong_bool(
		platform->window, "initWithContentRect:styleMask:backing:defer:",
		frame, style, FB_GFX3_DARWIN_BACKING_BUFFERED, NO);
	if (platform->window == nil)
		goto fail;
	platform->view = platform_darwin_id((id)platform_darwin_view_class,
		"alloc");
	platform->view = platform_darwin_id_rect(platform->view,
		"initWithFrame:", view_frame);
	if (platform->view == nil)
		goto fail;
	platform->layer = platform_darwin_id((id)layer_class, "layer");
	if (platform->layer == nil)
		goto fail;
	platform_darwin_void_bool(platform->view, "setWantsLayer:", YES);
	platform_darwin_void_id(platform->view, "setLayer:", platform->layer);
	platform_darwin_void_ulong(platform->view, "setAutoresizingMask:",
		FB_GFX3_DARWIN_VIEW_WIDTH_SIZABLE |
		FB_GFX3_DARWIN_VIEW_HEIGHT_SIZABLE);
	platform_darwin_void_id(platform->window, "setContentView:",
		platform->view);
	platform_darwin_void_bool(platform->window, "setReleasedWhenClosed:", NO);
	platform_darwin_void_id(platform->window, "setTitle:",
		platform_darwin_string(config->title));
	if (config->flags & FB_GFX3_WINDOW_RESIZABLE) {
		minimum_size.width = 8.0;
		minimum_size.height = 16.0;
		platform_darwin_void_size(platform->window, "setContentMinSize:",
			minimum_size);
	}
	if (!(config->flags & FB_GFX3_WINDOW_FULLSCREEN))
		platform_darwin_void(platform->window, "center");
	platform_darwin_void_bool(platform->window,
		"setAcceptsMouseMovedEvents:", YES);
	platform_darwin_bool_id(platform->window, "makeFirstResponder:",
		platform->view);
	platform_darwin_refresh_geometry(platform);
	platform_darwin_publish_window_info(platform);
	*destination = platform;
	return FB_GFX3_OK;

fail:
	if (platform->view != nil)
		platform_darwin_void(platform->view, "release");
	if (platform->window != nil) {
		platform_darwin_void(platform->window, "close");
		platform_darwin_void(platform->window, "release");
	}
	if (platform->pool != nil)
		platform_darwin_void(platform->pool, "release");
	free(platform);
	return FB_GFX3_FAILED;
}

static void platform_darwin_destroy(void *state)
{
	FB_GFX3_PLATFORM_DARWIN *platform =
		(FB_GFX3_PLATFORM_DARWIN *)state;

	if (platform == NULL)
		return;
	fb_gfx3_input_platform_window_info(platform->input, 0, 0, 0, 0, 0, 0);
	if (!platform->cursor_visible)
		platform_darwin_void((id)objc_getClass("NSCursor"), "unhide");
	if (platform->window != nil) {
		platform_darwin_void_id(platform->window, "orderOut:", nil);
		platform_darwin_void(platform->window, "close");
	}
	if (platform->view != nil)
		platform_darwin_void(platform->view, "release");
	if (platform->window != nil)
		platform_darwin_void(platform->window, "release");
	if (platform->pool != nil)
		platform_darwin_void(platform->pool, "release");
	free(platform);
}

static int platform_darwin_native_handles(void *state, uintptr_t *instance,
	uintptr_t *window)
{
	FB_GFX3_PLATFORM_DARWIN *platform =
		(FB_GFX3_PLATFORM_DARWIN *)state;

	if ((platform == NULL) || (instance == NULL) || (window == NULL) ||
	    (platform->window == nil) || (platform->layer == nil))
		return FB_GFX3_INVALID;
	*instance = (uintptr_t)platform->window;
	*window = (uintptr_t)platform->layer;
	return FB_GFX3_OK;
}

static int platform_darwin_client_size(void *state, uint32_t *width,
	uint32_t *height)
{
	FB_GFX3_PLATFORM_DARWIN *platform =
		(FB_GFX3_PLATFORM_DARWIN *)state;

	if ((platform == NULL) || (width == NULL) || (height == NULL))
		return FB_GFX3_INVALID;
	platform_darwin_refresh_geometry(platform);
	if ((platform->view_width == 0u) || (platform->view_height == 0u))
		return FB_GFX3_FAILED;
	*width = platform->view_width;
	*height = platform->view_height;
	return FB_GFX3_OK;
}

static int platform_darwin_desktop_info(ssize_t *width, ssize_t *height,
	ssize_t *depth, ssize_t *refresh)
{
	CGDirectDisplayID display;
	CGDisplayModeRef mode;
	double rate = 0.0;
	size_t display_width;
	size_t display_height;

	display = CGMainDisplayID();
	display_width = CGDisplayPixelsWide(display);
	display_height = CGDisplayPixelsHigh(display);
	if ((display_width == 0u) || (display_height == 0u) ||
	    (display_width > (size_t)SSIZE_MAX) ||
	    (display_height > (size_t)SSIZE_MAX))
		return FB_GFX3_FAILED;
	mode = CGDisplayCopyDisplayMode(display);
	if (mode != NULL) {
		rate = CGDisplayModeGetRefreshRate(mode);
		CGDisplayModeRelease(mode);
	}
	if (width != NULL)
		*width = (ssize_t)display_width;
	if (height != NULL)
		*height = (ssize_t)display_height;
	if (depth != NULL)
		*depth = 32;
	if (refresh != NULL)
		*refresh = rate > 0.0 ? (ssize_t)(rate + 0.5) : 0;
	return FB_GFX3_OK;
}

static int platform_darwin_show_window(void *state)
{
	FB_GFX3_PLATFORM_DARWIN *platform =
		(FB_GFX3_PLATFORM_DARWIN *)state;

	if (platform == NULL)
		return FB_GFX3_INVALID;
	if (!platform->shown) {
		platform_darwin_void_id(platform->window,
			"makeKeyAndOrderFront:", nil);
		platform_darwin_void_bool(platform->app,
			"activateIgnoringOtherApps:", YES);
		platform->shown = TRUE;
		platform_darwin_pump_events(platform);
	}
	return FB_GFX3_OK;
}

static int platform_darwin_set_window_title(void *state, const char *title)
{
	FB_GFX3_PLATFORM_DARWIN *platform =
		(FB_GFX3_PLATFORM_DARWIN *)state;

	if ((platform == NULL) || (title == NULL))
		return FB_GFX3_INVALID;
	platform_darwin_void_id(platform->window, "setTitle:",
		platform_darwin_string(title));
	return FB_GFX3_OK;
}

static int platform_darwin_probe_opengl(void)
{
	return FB_GFX3_UNSUPPORTED;
}

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_darwin = {
	"Cocoa Metal",
	platform_darwin_probe_opengl,
	platform_darwin_create_window,
	NULL,
	platform_darwin_native_handles,
	platform_darwin_destroy,
	NULL,
	platform_darwin_client_size,
	platform_darwin_desktop_info,
	NULL,
	platform_darwin_pump_events,
	platform_darwin_show_window,
	platform_darwin_set_window_title
};

#else

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_darwin = {
	"Cocoa Metal unavailable",
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
	return &__fb_gfx3_platform_darwin;
}

/* end of darwin/gfx3_platform.c */
