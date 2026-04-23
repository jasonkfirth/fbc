#include "../fb_gfx.h"
#include "fb_gfx_darwin.h"

#ifdef HOST_DARWIN

#include <CoreGraphics/CGColorSpace.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGDataProvider.h>
#include <CoreGraphics/CGDirectDisplay.h>
#include <CoreGraphics/CGGeometry.h>
#include <CoreGraphics/CGImage.h>
#include <limits.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>

#define FB_DARWIN_STYLE_MASK 15UL
#define FB_DARWIN_BACKING_BUFFERED 2UL
#define FB_DARWIN_ACTIVATION_POLICY_REGULAR 0L

#define FB_DARWIN_EVENT_LEFT_MOUSE_DOWN    1
#define FB_DARWIN_EVENT_LEFT_MOUSE_UP      2
#define FB_DARWIN_EVENT_RIGHT_MOUSE_DOWN   3
#define FB_DARWIN_EVENT_RIGHT_MOUSE_UP     4
#define FB_DARWIN_EVENT_MOUSE_MOVED        5
#define FB_DARWIN_EVENT_LEFT_MOUSE_DRAGGED 6
#define FB_DARWIN_EVENT_RIGHT_MOUSE_DRAGGED 7
#define FB_DARWIN_EVENT_MOUSE_ENTERED      8
#define FB_DARWIN_EVENT_MOUSE_EXITED       9
#define FB_DARWIN_EVENT_KEY_DOWN          10
#define FB_DARWIN_EVENT_KEY_UP            11
#define FB_DARWIN_EVENT_FLAGS_CHANGED     12
#define FB_DARWIN_EVENT_SCROLL_WHEEL      22
#define FB_DARWIN_EVENT_OTHER_MOUSE_DOWN  25
#define FB_DARWIN_EVENT_OTHER_MOUSE_UP    26
#define FB_DARWIN_EVENT_OTHER_MOUSE_DRAGGED 27

#define FB_DARWIN_MOD_CAPSLOCK (1UL << 16)
#define FB_DARWIN_MOD_SHIFT    (1UL << 17)
#define FB_DARWIN_MOD_CONTROL  (1UL << 18)
#define FB_DARWIN_MOD_OPTION   (1UL << 19)
#define FB_DARWIN_MOD_COMMAND  (1UL << 20)

#define FB_DARWIN_VIEW_ENCODING_RECT "v@:{CGRect={CGPoint=dd}{CGSize=dd}}"

FB_DARWIN_STATE fb_darwin;

static Class fb_darwin_view_class = Nil;

extern void fb_hPostKey(int key);

static SEL fb_sel(const char *name)
{
	return sel_registerName(name);
}

static id fb_msg_id(id obj, const char *sel_name)
{
	return ((id (*)(id, SEL))objc_msgSend)(obj, fb_sel(sel_name));
}

static id fb_msg_id_id(id obj, const char *sel_name, id arg)
{
	return ((id (*)(id, SEL, id))objc_msgSend)(obj, fb_sel(sel_name), arg);
}

static id fb_msg_id_cstr(id obj, const char *sel_name, const char *arg)
{
	return ((id (*)(id, SEL, const char *))objc_msgSend)(obj, fb_sel(sel_name), arg);
}

static id fb_msg_id_rect(id obj, const char *sel_name, CGRect rect)
{
	return ((id (*)(id, SEL, CGRect))objc_msgSend)(obj, fb_sel(sel_name), rect);
}

static id fb_msg_id_cgimage_size(id obj, const char *sel_name, CGImageRef image, CGSize size)
{
	return ((id (*)(id, SEL, CGImageRef, CGSize))objc_msgSend)(obj, fb_sel(sel_name), image, size);
}

static id fb_msg_id_rect_ulong_ulong_bool(id obj, const char *sel_name, CGRect rect, unsigned long style, unsigned long backing, BOOL defer_flag)
{
	return ((id (*)(id, SEL, CGRect, unsigned long, unsigned long, BOOL))objc_msgSend)(
		obj,
		fb_sel(sel_name),
		rect,
		style,
		backing,
		defer_flag
	);
}

static id fb_msg_id_ulong_id_id_bool(id obj, const char *sel_name, unsigned long mask, id date, id mode, BOOL dequeue)
{
	return ((id (*)(id, SEL, unsigned long, id, id, BOOL))objc_msgSend)(
		obj,
		fb_sel(sel_name),
		mask,
		date,
		mode,
		dequeue
	);
}

static id fb_msg_id_id_sel_id(id obj, const char *sel_name, id title, SEL action, id key)
{
	return ((id (*)(id, SEL, id, SEL, id))objc_msgSend)(
		obj,
		fb_sel(sel_name),
		title,
		action,
		key
	);
}

static void fb_msg_void(id obj, const char *sel_name)
{
	((void (*)(id, SEL))objc_msgSend)(obj, fb_sel(sel_name));
}

static void fb_msg_void_id(id obj, const char *sel_name, id arg)
{
	((void (*)(id, SEL, id))objc_msgSend)(obj, fb_sel(sel_name), arg);
}

static void fb_msg_void_id_id(id obj, const char *sel_name, id arg1, id arg2)
{
	((void (*)(id, SEL, id, id))objc_msgSend)(obj, fb_sel(sel_name), arg1, arg2);
}

static void fb_msg_void_bool(id obj, const char *sel_name, BOOL arg)
{
	((void (*)(id, SEL, BOOL))objc_msgSend)(obj, fb_sel(sel_name), arg);
}

static void fb_msg_void_long(id obj, const char *sel_name, long arg)
{
	((void (*)(id, SEL, long))objc_msgSend)(obj, fb_sel(sel_name), arg);
}

static void fb_msg_void_ulong(id obj, const char *sel_name, unsigned long arg)
{
	((void (*)(id, SEL, unsigned long))objc_msgSend)(obj, fb_sel(sel_name), arg);
}

static void fb_msg_void_point(id obj, const char *sel_name, CGPoint pt)
{
	((void (*)(id, SEL, CGPoint))objc_msgSend)(obj, fb_sel(sel_name), pt);
}

static void fb_msg_void_rect(id obj, const char *sel_name, CGRect rect)
{
	((void (*)(id, SEL, CGRect))objc_msgSend)(obj, fb_sel(sel_name), rect);
}

static void fb_msg_void_size(id obj, const char *sel_name, CGSize size)
{
	((void (*)(id, SEL, CGSize))objc_msgSend)(obj, fb_sel(sel_name), size);
}

static BOOL fb_msg_bool(id obj, const char *sel_name)
{
	return ((BOOL (*)(id, SEL))objc_msgSend)(obj, fb_sel(sel_name));
}

static BOOL fb_msg_bool_id(id obj, const char *sel_name, id arg)
{
	return ((BOOL (*)(id, SEL, id))objc_msgSend)(obj, fb_sel(sel_name), arg);
}

static unsigned long fb_msg_ulong(id obj, const char *sel_name)
{
	return ((unsigned long (*)(id, SEL))objc_msgSend)(obj, fb_sel(sel_name));
}

static double fb_msg_double(id obj, const char *sel_name)
{
	return ((double (*)(id, SEL))objc_msgSend)(obj, fb_sel(sel_name));
}

static const char *fb_msg_cstr(id obj, const char *sel_name)
{
	return ((const char *(*)(id, SEL))objc_msgSend)(obj, fb_sel(sel_name));
}

static CGPoint fb_msg_point(id obj, const char *sel_name)
{
	return ((CGPoint (*)(id, SEL))objc_msgSend)(obj, fb_sel(sel_name));
}

static CGPoint fb_msg_point_point_id(id obj, const char *sel_name, CGPoint pt, id view)
{
	return ((CGPoint (*)(id, SEL, CGPoint, id))objc_msgSend)(obj, fb_sel(sel_name), pt, view);
}

static CGRect fb_msg_rect(id obj, const char *sel_name)
{
	CGRect rect;

#if defined(__x86_64__) && !defined(__arm64__)
	((void (*)(CGRect *, id, SEL))objc_msgSend_stret)(&rect, obj, fb_sel(sel_name));
#else
	rect = ((CGRect (*)(id, SEL))objc_msgSend)(obj, fb_sel(sel_name));
#endif

	return rect;
}

static id fb_hDarwinMakeString(const char *text)
{
	Class NSStringClass = objc_getClass("NSString");

	if (!text)
		text = "FreeBASIC";

	return fb_msg_id_cstr((id)NSStringClass, "stringWithUTF8String:", text);
}

static int fb_hDarwinMouseMaskToButtons(unsigned long mask)
{
	int buttons = 0;

	if (mask & 0x1UL)
		buttons |= BUTTON_LEFT;
	if (mask & 0x2UL)
		buttons |= BUTTON_RIGHT;
	if (mask & 0x4UL)
		buttons |= BUTTON_MIDDLE;

	return buttons;
}

static int fb_hDarwinButtonNumberToMask(int button_number)
{
	switch (button_number) {
	case 0:
		return BUTTON_LEFT;
	case 1:
		return BUTTON_RIGHT;
	case 2:
		return BUTTON_MIDDLE;
	case 3:
		return BUTTON_X1;
	case 4:
		return BUTTON_X2;
	default:
		return 0;
	}
}

static int fb_hDarwinAsciiToScancode(unsigned char ch)
{
	switch (ch) {
	case 27:  return SC_ESCAPE;
	case '1': return SC_1;
	case '2': return SC_2;
	case '3': return SC_3;
	case '4': return SC_4;
	case '5': return SC_5;
	case '6': return SC_6;
	case '7': return SC_7;
	case '8': return SC_8;
	case '9': return SC_9;
	case '0': return SC_0;
	case '-': return SC_MINUS;
	case '=': return SC_EQUALS;
	case 8:
	case 127: return SC_BACKSPACE;
	case '\t': return SC_TAB;
	case 'q':
	case 'Q': return SC_Q;
	case 'w':
	case 'W': return SC_W;
	case 'e':
	case 'E': return SC_E;
	case 'r':
	case 'R': return SC_R;
	case 't':
	case 'T': return SC_T;
	case 'y':
	case 'Y': return SC_Y;
	case 'u':
	case 'U': return SC_U;
	case 'i':
	case 'I': return SC_I;
	case 'o':
	case 'O': return SC_O;
	case 'p':
	case 'P': return SC_P;
	case '[': return SC_LEFTBRACKET;
	case ']': return SC_RIGHTBRACKET;
	case '\r':
	case '\n': return SC_ENTER;
	case 'a':
	case 'A': return SC_A;
	case 's':
	case 'S': return SC_S;
	case 'd':
	case 'D': return SC_D;
	case 'f':
	case 'F': return SC_F;
	case 'g':
	case 'G': return SC_G;
	case 'h':
	case 'H': return SC_H;
	case 'j':
	case 'J': return SC_J;
	case 'k':
	case 'K': return SC_K;
	case 'l':
	case 'L': return SC_L;
	case ';': return SC_SEMICOLON;
	case '\'': return SC_QUOTE;
	case '`': return SC_TILDE;
	case '\\': return SC_BACKSLASH;
	case 'z':
	case 'Z': return SC_Z;
	case 'x':
	case 'X': return SC_X;
	case 'c':
	case 'C': return SC_C;
	case 'v':
	case 'V': return SC_V;
	case 'b':
	case 'B': return SC_B;
	case 'n':
	case 'N': return SC_N;
	case 'm':
	case 'M': return SC_M;
	case ',': return SC_COMMA;
	case '.': return SC_PERIOD;
	case '/': return SC_SLASH;
	case ' ': return SC_SPACE;
	default:  return 0;
	}
}

static int fb_hDarwinKeyCodeToScancode(unsigned short keycode)
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
	case 0x4B: return SC_ENTER;
	case 0x4C: return SC_ENTER;
	case 0x4E: return SC_PLUS;
	case 0x51: return SC_0;
	case 0x52: return SC_INSERT;
	case 0x53: return SC_END;
	case 0x54: return SC_DOWN;
	case 0x55: return SC_PAGEDOWN;
	case 0x56: return SC_LEFT;
	case 0x57: return SC_CLEAR;
	case 0x58: return SC_RIGHT;
	case 0x59: return SC_HOME;
	case 0x5A: return SC_UP;
	case 0x5B: return SC_PAGEUP;
	case 0x60: return SC_F5;
	case 0x61: return SC_F6;
	case 0x62: return SC_F7;
	case 0x63: return SC_F3;
	case 0x64: return SC_F8;
	case 0x65: return SC_F9;
	case 0x67: return SC_F11;
	case 0x6D: return SC_F10;
	case 0x6F: return SC_F12;
	case 0x72: return SC_INSERT;
	case 0x73: return SC_HOME;
	case 0x74: return SC_PAGEUP;
	case 0x75: return SC_DELETE;
	case 0x76: return SC_F4;
	case 0x77: return SC_END;
	case 0x78: return SC_F2;
	case 0x79: return SC_PAGEDOWN;
	case 0x7A: return SC_F1;
	case 0x7B: return SC_LEFT;
	case 0x7C: return SC_RIGHT;
	case 0x7D: return SC_DOWN;
	case 0x7E: return SC_UP;
	default:   return 0;
	}
}

static int fb_hDarwinEventKey(id event)
{
	id chars;
	const char *utf8;
	int ascii = 0;
	int scancode;
	unsigned short keycode;

	chars = fb_msg_id(event, "characters");
	if (chars) {
		utf8 = fb_msg_cstr(chars, "UTF8String");
		if (utf8 && utf8[0] != '\0' && utf8[1] == '\0') {
			ascii = (unsigned char)utf8[0];
			if (ascii == 127)
				ascii = KEY_BACKSPACE;
		}
	}

	keycode = (unsigned short)fb_msg_ulong(event, "keyCode");
	scancode = fb_hDarwinKeyCodeToScancode(keycode);
	if (!scancode && ascii)
		scancode = fb_hDarwinAsciiToScancode((unsigned char)ascii);

	if (ascii > 0 && ascii <= 0xFF)
		return ascii;

	return fb_hScancodeToExtendedKey(scancode);
}

static void fb_hDarwinQueryDesktop(void)
{
	CGDirectDisplayID display_id = CGMainDisplayID();

	fb_darwin.desktop_width = (int)CGDisplayPixelsWide(display_id);
	fb_darwin.desktop_height = (int)CGDisplayPixelsHigh(display_id);
}

static void fb_hDarwinRefreshLayout(void)
{
	CGRect bounds;
	int scale_x;
	int scale_y;
	int scale;

	if (!fb_darwin.view)
		return;

	bounds = fb_msg_rect((id)fb_darwin.view, "bounds");
	fb_darwin.view_width = (int)bounds.size.width;
	fb_darwin.view_height = (int)bounds.size.height;

	if (fb_darwin.width <= 0 || fb_darwin.height <= 0) {
		fb_darwin.scale = 1;
		fb_darwin.draw_offset_x = 0;
		fb_darwin.draw_offset_y = 0;
		fb_darwin.draw_width = fb_darwin.view_width;
		fb_darwin.draw_height = fb_darwin.view_height;
		return;
	}

	scale_x = fb_darwin.view_width / fb_darwin.width;
	scale_y = fb_darwin.view_height / fb_darwin.height;
	scale = (scale_x < scale_y) ? scale_x : scale_y;
	if (scale < 1)
		scale = 1;

	fb_darwin.scale = scale;
	fb_darwin.draw_width = fb_darwin.width * scale;
	fb_darwin.draw_height = fb_darwin.height * scale;
	fb_darwin.draw_offset_x = (fb_darwin.view_width - fb_darwin.draw_width) / 2;
	fb_darwin.draw_offset_y = (fb_darwin.view_height - fb_darwin.draw_height) / 2;
}

static void fb_hDarwinPostCloseEvent(void)
{
	EVENT e;

	if (fb_darwin.quit_posted)
		return;

	memset(&e, 0, sizeof(e));
	e.type = EVENT_WINDOW_CLOSE;
	fb_hPostKey(KEY_QUIT);
	fb_hPostEvent(&e);
	fb_darwin.quit_posted = TRUE;
	exit(0);
}

static void fb_hDarwinPostFocusEvent(int gained_focus)
{
	EVENT e;

	if (fb_darwin.has_focus == gained_focus)
		return;

	memset(&e, 0, sizeof(e));
	e.type = gained_focus ? EVENT_WINDOW_GOT_FOCUS : EVENT_WINDOW_LOST_FOCUS;
	fb_hPostEvent(&e);
	fb_darwin.has_focus = gained_focus;
}

static void fb_hDarwinViewPointToFramebuffer(CGPoint pt, int *outx, int *outy)
{
	int rel_x;
	int rel_y;
	int fx;
	int fy;

	fb_hDarwinRefreshLayout();

	rel_x = (int)pt.x - fb_darwin.draw_offset_x;
	rel_y = (int)pt.y - fb_darwin.draw_offset_y;

	if (rel_x < 0)
		rel_x = 0;
	if (rel_y < 0)
		rel_y = 0;
	if (rel_x >= fb_darwin.draw_width)
		rel_x = fb_darwin.draw_width - 1;
	if (rel_y >= fb_darwin.draw_height)
		rel_y = fb_darwin.draw_height - 1;

	if (fb_darwin.scale > 0) {
		fx = rel_x / fb_darwin.scale;
		fy = rel_y / fb_darwin.scale;
	} else {
		fx = rel_x;
		fy = rel_y;
	}

	if (fx < 0)
		fx = 0;
	if (fy < 0)
		fy = 0;
	if (fx >= fb_darwin.width)
		fx = fb_darwin.width - 1;
	if (fy >= fb_darwin.height)
		fy = fb_darwin.height - 1;

	if (outx)
		*outx = fx;
	if (outy)
		*outy = fy;
}

static void fb_hDarwinWindowPointToFramebuffer(CGPoint pt, int *outx, int *outy)
{
	CGPoint view_pt;

	if (!fb_darwin.view) {
		if (outx)
			*outx = 0;
		if (outy)
			*outy = 0;
		return;
	}

	view_pt = fb_msg_point_point_id((id)fb_darwin.view, "convertPoint:fromView:", pt, nil);
	fb_hDarwinViewPointToFramebuffer(view_pt, outx, outy);
}

static void fb_hDarwinPostMouseMove(int x, int y)
{
	EVENT e;

	memset(&e, 0, sizeof(e));
	e.type = EVENT_MOUSE_MOVE;
	e.x = x;
	e.y = y;
	e.dx = x - fb_darwin.mouse_x;
	e.dy = y - fb_darwin.mouse_y;
	fb_hPostEvent(&e);

	fb_darwin.mouse_x = x;
	fb_darwin.mouse_y = y;
}

static void fb_hDarwinHandleMouseButton(id event, int pressed)
{
	EVENT e;
	CGPoint location;
	int x;
	int y;
	int button_mask;
	int button_number;

	location = fb_msg_point(event, "locationInWindow");
	fb_hDarwinWindowPointToFramebuffer(location, &x, &y);
	fb_hDarwinPostMouseMove(x, y);

	button_number = (int)fb_msg_ulong(event, "buttonNumber");
	button_mask = fb_hDarwinButtonNumberToMask(button_number);
	if (!button_mask)
		return;

	if (pressed)
		fb_darwin.mouse_buttons |= button_mask;
	else
		fb_darwin.mouse_buttons &= ~button_mask;

	memset(&e, 0, sizeof(e));
	e.type = pressed ? EVENT_MOUSE_BUTTON_PRESS : EVENT_MOUSE_BUTTON_RELEASE;
	e.button = button_mask;
	fb_hPostEvent(&e);
}

static void fb_hDarwinHandleScrollWheel(id event)
{
	EVENT e;
	double delta_y;

	delta_y = fb_msg_double(event, "deltaY");
	if (delta_y == 0.0)
		return;

	fb_darwin.mouse_z += (delta_y > 0.0) ? 1 : -1;

	memset(&e, 0, sizeof(e));
	e.type = EVENT_MOUSE_WHEEL;
	e.z = fb_darwin.mouse_z;
	fb_hPostEvent(&e);
}

static void fb_hDarwinHandleKeyEvent(id event, int pressed)
{
	EVENT e;
	int scancode;
	int key;
	unsigned short keycode;
	int repeated;

	keycode = (unsigned short)fb_msg_ulong(event, "keyCode");
	scancode = fb_hDarwinKeyCodeToScancode(keycode);
	key = fb_hDarwinEventKey(event);
	repeated = pressed ? fb_msg_bool(event, "isARepeat") : FALSE;

	if (__fb_gfx && scancode > 0 && scancode < 128)
		__fb_gfx->key[scancode] = pressed ? TRUE : FALSE;

	if (pressed && key)
		fb_hPostKey(key);

	memset(&e, 0, sizeof(e));
	e.type = pressed ? (repeated ? EVENT_KEY_REPEAT : EVENT_KEY_PRESS) : EVENT_KEY_RELEASE;
	e.scancode = scancode;
	e.ascii = (key >= 0 && key <= 0xFF) ? key : 0;
	fb_hPostEvent(&e);
}

static void fb_hDarwinHandleModifierEvent(id event)
{
	EVENT e;
	unsigned short keycode;
	unsigned long flags;
	int scancode;
	int pressed;

	keycode = (unsigned short)fb_msg_ulong(event, "keyCode");
	flags = fb_msg_ulong(event, "modifierFlags");
	scancode = fb_hDarwinKeyCodeToScancode(keycode);
	if (!scancode)
		return;

	switch (keycode) {
	case 0x38:
	case 0x3C:
		pressed = (flags & FB_DARWIN_MOD_SHIFT) != 0;
		break;
	case 0x3B:
	case 0x3E:
		pressed = (flags & FB_DARWIN_MOD_CONTROL) != 0;
		break;
	case 0x3A:
	case 0x3D:
		pressed = (flags & FB_DARWIN_MOD_OPTION) != 0;
		break;
	case 0x37:
		pressed = (flags & FB_DARWIN_MOD_COMMAND) != 0;
		break;
	case 0x39:
		pressed = (flags & FB_DARWIN_MOD_CAPSLOCK) != 0;
		break;
	default:
		pressed = 0;
		break;
	}

	if (__fb_gfx && scancode > 0 && scancode < 128)
		__fb_gfx->key[scancode] = pressed ? TRUE : FALSE;

	memset(&e, 0, sizeof(e));
	e.type = pressed ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE;
	e.scancode = scancode;
	e.ascii = 0;
	fb_hPostEvent(&e);
}

static void fb_hDarwinHandleEvent(id event)
{
	CGPoint location;
	int x;
	int y;
	unsigned long type;

	type = fb_msg_ulong(event, "type");

	switch (type) {
	case FB_DARWIN_EVENT_KEY_DOWN:
		fb_hDarwinHandleKeyEvent(event, TRUE);
		break;

	case FB_DARWIN_EVENT_KEY_UP:
		fb_hDarwinHandleKeyEvent(event, FALSE);
		break;

	case FB_DARWIN_EVENT_FLAGS_CHANGED:
		fb_hDarwinHandleModifierEvent(event);
		break;

	case FB_DARWIN_EVENT_LEFT_MOUSE_DOWN:
	case FB_DARWIN_EVENT_RIGHT_MOUSE_DOWN:
	case FB_DARWIN_EVENT_OTHER_MOUSE_DOWN:
		fb_hDarwinHandleMouseButton(event, TRUE);
		break;

	case FB_DARWIN_EVENT_LEFT_MOUSE_UP:
	case FB_DARWIN_EVENT_RIGHT_MOUSE_UP:
	case FB_DARWIN_EVENT_OTHER_MOUSE_UP:
		fb_hDarwinHandleMouseButton(event, FALSE);
		break;

	case FB_DARWIN_EVENT_MOUSE_MOVED:
	case FB_DARWIN_EVENT_LEFT_MOUSE_DRAGGED:
	case FB_DARWIN_EVENT_RIGHT_MOUSE_DRAGGED:
	case FB_DARWIN_EVENT_OTHER_MOUSE_DRAGGED:
		location = fb_msg_point(event, "locationInWindow");
		fb_hDarwinWindowPointToFramebuffer(location, &x, &y);
		fb_hDarwinPostMouseMove(x, y);
		break;

	case FB_DARWIN_EVENT_MOUSE_ENTERED:
		memset(&x, 0, sizeof(x));
		break;

	case FB_DARWIN_EVENT_MOUSE_EXITED:
		{
			EVENT e;
			memset(&e, 0, sizeof(e));
			e.type = EVENT_MOUSE_EXIT;
			fb_hPostEvent(&e);
		}
		break;

	case FB_DARWIN_EVENT_SCROLL_WHEEL:
		fb_hDarwinHandleScrollWheel(event);
		break;
	}
}

static void fb_hDarwinPumpEvents(void)
{
	id event;
	BOOL is_key_window;

	if (!fb_darwin.app)
		return;

	while ((event = fb_msg_id_ulong_id_id_bool(
		(id)fb_darwin.app,
		"nextEventMatchingMask:untilDate:inMode:dequeue:",
		ULONG_MAX,
		nil,
		(id)fb_darwin.run_loop_mode,
		YES
	)) != nil) {
		fb_hDarwinHandleEvent(event);
		fb_msg_void_id((id)fb_darwin.app, "sendEvent:", event);
	}

	fb_msg_void((id)fb_darwin.app, "updateWindows");

	if (fb_darwin.window) {
		is_key_window = fb_msg_bool((id)fb_darwin.window, "isKeyWindow");
		fb_hDarwinPostFocusEvent(is_key_window ? TRUE : FALSE);

		if (!fb_msg_bool((id)fb_darwin.window, "isVisible"))
			fb_hDarwinPostCloseEvent();
	}
}

static void fb_hDarwinDrawCurrentFramebuffer(CGContextRef ctx, CGRect bounds)
{
	CGColorSpaceRef color_space;
	CGDataProviderRef provider;
	CGImageRef image;
	CGRect dest_rect;
	CGRect cg_dest_rect;
	size_t data_size;

	CGContextSetRGBFillColor(ctx, 0.0, 0.0, 0.0, 1.0);
	CGContextFillRect(ctx, bounds);

	if (!__fb_gfx || !__fb_gfx->framebuffer)
		return;

	fb_hDarwinRefreshLayout();

	data_size = (size_t)__fb_gfx->pitch * (size_t)__fb_gfx->h;
	color_space = CGColorSpaceCreateDeviceRGB();
	provider = CGDataProviderCreateWithData(NULL, __fb_gfx->framebuffer, data_size, NULL);
	if (!color_space || !provider) {
		if (provider)
			CGDataProviderRelease(provider);
		if (color_space)
			CGColorSpaceRelease(color_space);
		return;
	}

	image = CGImageCreate(
		(size_t)__fb_gfx->w,
		(size_t)__fb_gfx->h,
		8,
		32,
		(size_t)__fb_gfx->pitch,
		color_space,
		kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,
		provider,
		NULL,
		FALSE,
		kCGRenderingIntentDefault
	);

	CGDataProviderRelease(provider);
	CGColorSpaceRelease(color_space);

	if (!image)
		return;

	dest_rect = CGRectMake(
		(CGFloat)fb_darwin.draw_offset_x,
		(CGFloat)fb_darwin.draw_offset_y,
		(CGFloat)fb_darwin.draw_width,
		(CGFloat)fb_darwin.draw_height
	);

	cg_dest_rect = CGRectMake(
		dest_rect.origin.x,
		bounds.size.height - dest_rect.origin.y - dest_rect.size.height,
		dest_rect.size.width,
		dest_rect.size.height
	);

	CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
	CGContextSaveGState(ctx);
	CGContextTranslateCTM(
		ctx,
		cg_dest_rect.origin.x,
		cg_dest_rect.origin.y + cg_dest_rect.size.height
	);
	CGContextScaleCTM(ctx, 1.0, -1.0);
	CGContextDrawImage(
		ctx,
		CGRectMake(0.0, 0.0, cg_dest_rect.size.width, cg_dest_rect.size.height),
		image
	);
	CGContextRestoreGState(ctx);
	CGImageRelease(image);
}

static BOOL fb_darwin_view_is_flipped(id self, SEL cmd)
{
	(void)self;
	(void)cmd;
	return YES;
}

static BOOL fb_darwin_view_accepts_first_responder(id self, SEL cmd)
{
	(void)self;
	(void)cmd;
	return YES;
}

static void fb_darwin_view_view_did_move_to_window(id self, SEL cmd)
{
	id window;

	(void)cmd;

	window = fb_msg_id(self, "window");
	if (!window)
		return;

	fb_msg_void_bool(window, "setAcceptsMouseMovedEvents:", YES);
	fb_msg_bool_id(window, "makeFirstResponder:", self);
}

static void fb_darwin_view_draw_rect(id self, SEL cmd, CGRect dirty_rect)
{
	id graphics_context;
	CGContextRef cg;
	CGRect bounds;

	(void)cmd;
	(void)dirty_rect;

	graphics_context = fb_msg_id((id)objc_getClass("NSGraphicsContext"), "currentContext");
	if (!graphics_context)
		return;

	cg = (CGContextRef)fb_msg_id(graphics_context, "CGContext");
	if (!cg)
		return;

	bounds = fb_msg_rect(self, "bounds");
	fb_hDarwinDrawCurrentFramebuffer(cg, bounds);
}

static void fb_hDarwinEnsureClasses(void)
{
	Class NSViewClass;

	if (fb_darwin_view_class != Nil)
		return;

	NSViewClass = objc_getClass("NSView");
	if (!NSViewClass)
		return;

	fb_darwin_view_class = objc_allocateClassPair(NSViewClass, "FBDarwinView", 0);
	if (!fb_darwin_view_class)
		return;

	class_addMethod(fb_darwin_view_class, fb_sel("isFlipped"), (IMP)fb_darwin_view_is_flipped, "B@:");
	class_addMethod(fb_darwin_view_class, fb_sel("acceptsFirstResponder"), (IMP)fb_darwin_view_accepts_first_responder, "B@:");
	class_addMethod(fb_darwin_view_class, fb_sel("viewDidMoveToWindow"), (IMP)fb_darwin_view_view_did_move_to_window, "v@:");
	class_addMethod(fb_darwin_view_class, fb_sel("drawRect:"), (IMP)fb_darwin_view_draw_rect, FB_DARWIN_VIEW_ENCODING_RECT);

	objc_registerClassPair(fb_darwin_view_class);
}

static void fb_hDarwinEnsureMenu(char *title)
{
	Class NSMenuClass;
	Class NSMenuItemClass;
	id main_menu;
	id app_item;
	id app_menu;
	id quit_item;
	id empty_title;
	id app_title;
	id quit_title;
	id quit_key;

	if (fb_darwin.main_menu)
		return;

	NSMenuClass = objc_getClass("NSMenu");
	NSMenuItemClass = objc_getClass("NSMenuItem");
	if (!NSMenuClass || !NSMenuItemClass)
		return;

	empty_title = fb_hDarwinMakeString("");
	app_title = fb_hDarwinMakeString(title ? title : "FreeBASIC");
	quit_title = fb_hDarwinMakeString("Quit");
	quit_key = fb_hDarwinMakeString("q");

	main_menu = fb_msg_id((id)NSMenuClass, "alloc");
	main_menu = fb_msg_id_id(main_menu, "initWithTitle:", empty_title);

	app_item = fb_msg_id((id)NSMenuItemClass, "alloc");
	app_item = fb_msg_id_id_sel_id(app_item, "initWithTitle:action:keyEquivalent:", empty_title, (SEL)0, empty_title);

	app_menu = fb_msg_id((id)NSMenuClass, "alloc");
	app_menu = fb_msg_id_id(app_menu, "initWithTitle:", app_title);

	quit_item = fb_msg_id((id)NSMenuItemClass, "alloc");
	quit_item = fb_msg_id_id_sel_id(quit_item, "initWithTitle:action:keyEquivalent:", quit_title, fb_sel("terminate:"), quit_key);

	fb_msg_void_id(main_menu, "addItem:", app_item);
	fb_msg_void_id(app_menu, "addItem:", quit_item);
	fb_msg_void_id_id(main_menu, "setSubmenu:forItem:", app_menu, app_item);
	fb_msg_void_id((id)fb_darwin.app, "setMainMenu:", main_menu);

	fb_darwin.main_menu = main_menu;
	fb_darwin.app_menu = app_menu;
}

int fb_hDarwinInit(char *title, int w, int h, int depth, int refresh_rate, int flags)
{
	Class NSApplicationClass;
	Class NSWindowClass;
	CGRect frame;
	CGSize min_size;
	id app;
	id window;
	id view;

	if (flags & DRIVER_OPENGL)
		return -1;

	if (depth != 32)
		return -1;

	if (fb_darwin.initialized)
		fb_hDarwinExit();

	memset(&fb_darwin, 0, sizeof(fb_darwin));
	fb_hDarwinQueryDesktop();
	fb_hDarwinEnsureClasses();

	NSApplicationClass = objc_getClass("NSApplication");
	NSWindowClass = objc_getClass("NSWindow");
	if (!NSApplicationClass || !NSWindowClass || fb_darwin_view_class == Nil)
		return -1;

	app = fb_msg_id((id)NSApplicationClass, "sharedApplication");
	if (!app)
		return -1;

	fb_darwin.app = app;
	fb_darwin.run_loop_mode = fb_hDarwinMakeString("kCFRunLoopDefaultMode");

	fb_msg_void_long(app, "setActivationPolicy:", FB_DARWIN_ACTIVATION_POLICY_REGULAR);
	fb_hDarwinEnsureMenu(title);
	fb_msg_void(app, "finishLaunching");

	frame = CGRectMake(0.0, 0.0, (CGFloat)w, (CGFloat)h);

	window = fb_msg_id((id)NSWindowClass, "alloc");
	window = fb_msg_id_rect_ulong_ulong_bool(
		window,
		"initWithContentRect:styleMask:backing:defer:",
		frame,
		FB_DARWIN_STYLE_MASK,
		FB_DARWIN_BACKING_BUFFERED,
		NO
	);
	if (!window)
		return -1;

	view = fb_msg_id((id)fb_darwin_view_class, "alloc");
	view = fb_msg_id_rect(view, "initWithFrame:", frame);
	if (!view) {
		fb_msg_void(window, "close");
		return -1;
	}

	min_size.width = (CGFloat)w;
	min_size.height = (CGFloat)h;

	fb_msg_void_id(window, "setContentView:", view);
	fb_msg_void_bool(window, "setReleasedWhenClosed:", NO);
	fb_msg_void_size(window, "setContentMinSize:", min_size);
	fb_msg_void_id(window, "setTitle:", fb_hDarwinMakeString(title));
	fb_msg_void(window, "center");
	fb_msg_void_id(window, "makeKeyAndOrderFront:", nil);
	fb_msg_void_bool(window, "setAcceptsMouseMovedEvents:", YES);
	fb_msg_bool_id(window, "makeFirstResponder:", view);
	fb_msg_void_bool(app, "activateIgnoringOtherApps:", YES);

	fb_darwin.window = window;
	fb_darwin.view = view;
	fb_darwin.width = w;
	fb_darwin.height = h;
	fb_darwin.depth = depth;
	fb_darwin.refresh_rate = refresh_rate;
	fb_darwin.flags = flags;
	fb_darwin.mouse_cursor = 1;
	fb_darwin.initialized = TRUE;

	fb_hDarwinRefreshLayout();
	fb_hDarwinPumpEvents();

	return 0;
}

void fb_hDarwinExit(void)
{
	if (fb_darwin.view) {
		fb_msg_void((id)fb_darwin.view, "release");
		fb_darwin.view = NULL;
	}

	if (fb_darwin.window) {
		fb_msg_void((id)fb_darwin.window, "close");
		fb_msg_void((id)fb_darwin.window, "release");
		fb_darwin.window = NULL;
	}

	memset(&fb_darwin, 0, sizeof(fb_darwin));
}

void fb_hDarwinUpdate(void)
{
	if (!fb_darwin.initialized || !fb_darwin.view)
		return;

	fb_hDarwinRefreshLayout();
	fb_msg_void_bool((id)fb_darwin.view, "setNeedsDisplay:", YES);
	fb_hDarwinPumpEvents();
}

void fb_hDarwinPollEvents(void)
{
	fb_hDarwinPumpEvents();
}

void fb_hDarwinLock(void)
{
}

void fb_hDarwinUnlock(void)
{
	fb_hDarwinUpdate();
}

void fb_hDarwinSetPalette(int index, int r, int g, int b)
{
	(void)index;
	(void)r;
	(void)b;
	(void)g;
}

void fb_hDarwinWaitVSync(void)
{
}

int fb_hDarwinGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	if (x)
		*x = fb_darwin.mouse_x;
	if (y)
		*y = fb_darwin.mouse_y;
	if (z)
		*z = fb_darwin.mouse_z;
	if (buttons)
		*buttons = fb_darwin.mouse_buttons;
	if (clip)
		*clip = fb_darwin.mouse_clip;

	return 0;
}

void fb_hDarwinSetMouse(int x, int y, int cursor, int clip)
{
	fb_darwin.mouse_x = x;
	fb_darwin.mouse_y = y;
	fb_darwin.mouse_clip = clip;

	if (fb_darwin.mouse_cursor != cursor) {
		if (cursor)
			fb_msg_void((id)objc_getClass("NSCursor"), "unhide");
		else
			fb_msg_void((id)objc_getClass("NSCursor"), "hide");
	}

	fb_darwin.mouse_cursor = cursor;
}

void fb_hDarwinSetWindowTitle(char *title)
{
	if (!fb_darwin.window)
		return;

	fb_msg_void_id((id)fb_darwin.window, "setTitle:", fb_hDarwinMakeString(title));
}

int fb_hDarwinSetWindowPos(int x, int y)
{
	CGPoint pt;

	if (!fb_darwin.window)
		return -1;

	pt.x = (CGFloat)x;
	pt.y = (CGFloat)y;
	fb_msg_void_point((id)fb_darwin.window, "setFrameOrigin:", pt);

	return 0;
}

static int fb_hDarwinModeExists(const int *modes, int count, int packed)
{
	int i;

	for (i = 0; i < count; ++i) {
		if (modes[i] == packed)
			return TRUE;
	}

	return FALSE;
}

static void fb_hDarwinAddMode(int *modes, int *count, int maxcount, int w, int h)
{
	int packed;

	if (!modes || !count || w <= 0 || h <= 0)
		return;

	packed = (w << 16) | (h & 0xFFFF);
	if (fb_hDarwinModeExists(modes, *count, packed))
		return;

	if (*count >= maxcount)
		return;

	modes[*count] = packed;
	(*count)++;
}

int *fb_hDarwinFetchModes(int depth, int *size)
{
	int *modes;
	int count = 0;
	int capacity = 32;

	(void)depth;

	if (!size)
		return NULL;

	*size = 0;
	modes = (int *)calloc((size_t)capacity, sizeof(int));
	if (!modes)
		return NULL;

	fb_hDarwinQueryDesktop();
	fb_hDarwinAddMode(modes, &count, capacity, fb_darwin.desktop_width, fb_darwin.desktop_height);
	fb_hDarwinAddMode(modes, &count, capacity, 320, 200);
	fb_hDarwinAddMode(modes, &count, capacity, 320, 240);
	fb_hDarwinAddMode(modes, &count, capacity, 400, 300);
	fb_hDarwinAddMode(modes, &count, capacity, 512, 384);
	fb_hDarwinAddMode(modes, &count, capacity, 640, 400);
	fb_hDarwinAddMode(modes, &count, capacity, 640, 480);
	fb_hDarwinAddMode(modes, &count, capacity, 800, 600);
	fb_hDarwinAddMode(modes, &count, capacity, 1024, 768);
	fb_hDarwinAddMode(modes, &count, capacity, 1280, 720);
	fb_hDarwinAddMode(modes, &count, capacity, 1280, 1024);
	fb_hDarwinAddMode(modes, &count, capacity, 1600, 900);
	fb_hDarwinAddMode(modes, &count, capacity, 1920, 1080);

	if (count == 0) {
		free(modes);
		return NULL;
	}

	*size = count;
	return modes;
}

int fb_hDarwinScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	fb_hDarwinQueryDesktop();

	if (width)
		*width = fb_darwin.desktop_width;
	if (height)
		*height = fb_darwin.desktop_height;
	if (depth)
		*depth = 32;
	if (refresh)
		*refresh = 0;

	return (fb_darwin.desktop_width > 0 && fb_darwin.desktop_height > 0) ? 1 : 0;
}

#endif
