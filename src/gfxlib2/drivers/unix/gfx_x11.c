/* FreeBASIC gfxlib2: gfx_x11.c
 * Purpose: Share native window handling between X11 and OpenGL drivers.
 * Responsibilities: Window lifecycle, input and bounded Xdnd reception.
 * This file does not open dropped files or implement drawing primitives.
 */

#include <stdint.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>

#ifndef DISABLE_X11

#include "../../fb_gfx.h"
#include "fb_gfx_x11.h"
#include <X11/Xatom.h>
#ifdef HOST_LINUX
#include "../linux/fb_linux_maliit.h"
#endif
#include "../../../rtlib/drivers/unix/fb_private_scancodes_x11.h"
#include <pthread.h>
#if defined(__has_include)
#if __has_include(<X11/extensions/XInput2.h>)
#include <X11/extensions/XInput2.h>
#define FB_X11_HAS_XINPUT2 1
#endif
#endif
#ifndef FB_X11_HAS_XINPUT2
#define FB_X11_HAS_XINPUT2 0
#endif
#if FB_X11_HAS_XINPUT2
#include <dlfcn.h>
#endif
#include "fb_x11_drop.h"
#include "fb_x11_key_text.h"

static FB_X11_DROP drop_windows[3];

static int x11_drop_enabled(void *user)
{
	int enabled;
	(void)user;
	EVENT_LOCK();
	enabled = __fb_gfx->drops.enabled;
	EVENT_UNLOCK();
	return enabled;
}

static int x11_drop_submit(void *user, int kind, const char *data,
	size_t bytes, int count)
{
	(void)user;
	return fb_hGfxDropSubmit(kind, data, bytes, count);
}

/* Horizontal scroll wheel (6 == left, 7 == right)
   X headers do not define these, as X was not designed for this,
   but they are still generated, on modern systems anyways. There probably
   is no guarantee that 6 and 7 will always be hwheel, but as far as gfxlib2
   is concerned it should be ok to assume 6 and 7 to be hwheel instead of
   treating them as regular buttondown. */
#ifndef Button6
#define Button6			6
#endif
#ifndef Button7
#define Button7			7
#endif

X11DRIVER fb_x11;

static pthread_t thread;
static pthread_mutex_t mutex;
static pthread_mutex_t mouse_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond;

static Drawable root_window;
static Atom wm_delete_window, wm_intern_hints;
static Colormap color_map = None;
static Time last_click_time = 0;
static int orig_size, target_size, current_size, real_h;
static int orig_rate, target_rate;
static Rotation orig_rotation;
static Cursor blank_cursor, arrow_cursor = None;
static int is_running = FALSE, has_focus, cursor_shown, xlib_inited = FALSE;
/* Pointer entry and presses also set has_focus for legacy mouse handling.
 * Track the state reported to ScreenEvent separately: pointer activity must
 * not suppress a subsequent FocusIn after we reported the window inactive. */
static int focus_notified;
/* Keep a one-shot latch of press events in case polling misses a short click. */
static int mouse_x, mouse_y, mouse_wheel, mouse_hwheel, mouse_buttons, mouse_latched_buttons, mouse_on;
static int mouse_state_initialized;
static int mouse_x_root, mouse_y_root;

static void hSetWindowFocus(int focused, EVENT *event)
{
	if (focused) {
		has_focus = TRUE;
		if (!focus_notified)
			event->type = EVENT_WINDOW_GOT_FOCUS;
		focus_notified = TRUE;
	} else {
		has_focus = FALSE;
		focus_notified = FALSE;
		event->type = EVENT_WINDOW_LOST_FOCUS;
	}
}

/* ------------------------------------------------------------------------- */
/* Optional XInput2 touch input                                               */
/* ------------------------------------------------------------------------- */

#if FB_X11_HAS_XINPUT2
#define FB_X11_MAX_TOUCH_POINTS 16

typedef struct FB_X11_TOUCH_POINT {
	int active;
	int id;
	int source_id;
	int detail_id;
	int x;
	int y;
} FB_X11_TOUCH_POINT;

typedef int (*FB_XI_QUERY_VERSION)(Display *, int *, int *);
typedef int (*FB_XI_SELECT_EVENTS)(Display *, Window, XIEventMask *, int);

typedef struct FB_X11_TOUCH_API {
	void *library;
	FB_XI_QUERY_VERSION query_version;
	FB_XI_SELECT_EVENTS select_events;
	int extension_opcode;
	int enabled;
} FB_X11_TOUCH_API;

static FB_X11_TOUCH_API touch_api;
static FB_X11_TOUCH_POINT touch_points[FB_X11_MAX_TOUCH_POINTS];
static int touch_primary_id = -1;
static int touch_primary_mouse_down;
static int touch_next_id;
/* The event thread writes contacts while applications poll them on another thread. */
static pthread_mutex_t touch_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static int calc_comp_height( int h )
{
	if( h < 240 )
		return 240;
	else if( h < 480 )
		return 480;
	else if( h < 600 )
		return 600;
	else if( h < 768 )
		return 768;
	else if( h < 1024 )
		return 1024;
	else
		return h;
}

static int key_repeated(XEvent *event)
{
	/* this function is shamelessly copied from SDL, which
	 * shamelessly copied it from yet another place :P
	 */
	XEvent peek_event;

	if (XPending(fb_x11.display)) {
		XPeekEvent(fb_x11.display, &peek_event);
		if ((peek_event.type == KeyPress) && (peek_event.xkey.keycode == event->xkey.keycode) &&
		    ((peek_event.xkey.time - event->xkey.time) < 2)) {
			XNextEvent(fb_x11.display, &peek_event);
			return TRUE;
		}
	}
	return FALSE;
}

static int translate_key(XEvent *event, int scancode, uint32_t *text_scalar)
{
	char text[8];
	int length;
	uint32_t scalar;

	length = XLookupString(&event->xkey, text, sizeof(text), NULL, NULL);
	scalar = (length > 0) ? fb_hX11DecodeKeyText(text, (size_t)length) : 0u;
	*text_scalar = 0u;
	if ((length == 1) && (text[0] == '\0'))
		return 0;
	if (scalar) {
		/* Preserve the extended DELETE key used by INKEY and SCREENEVENT. */
		if (scalar == 0x7Fu)
			return KEY_DEL;
		*text_scalar = scalar;
		/* Larger Unicode scalars cannot be represented by legacy INKEY. */
		return (scalar <= 0xFFu) ? (int)scalar : 0;
	}
	return fb_hScancodeToExtendedKey(scancode);
}

static void hOnAltEnter( void )
{
	fb_x11.exit();
	fb_x11.flags ^= DRIVER_FULLSCREEN;
	if (fb_x11.init()) {
		fb_x11.exit();
		fb_x11.flags ^= DRIVER_FULLSCREEN;
		fb_x11.init();
	}
	fb_hRestorePalette();
	fb_hMemSet(__fb_gfx->key, FALSE, 128);
}

void fb_hX11RefreshLayout(int view_w, int view_h)
{
#ifndef GFXLIB_NEVERSCALE
	int scale_x;
	int scale_y;
#endif
	int scale;

	if (view_w <= 0)
		view_w = fb_x11.w;
	if (view_h <= 0)
		view_h = fb_x11.h;

	if (fb_x11.content_w <= 0)
		fb_x11.content_w = fb_x11.w;
	if (fb_x11.content_h <= 0)
		fb_x11.content_h = fb_x11.h;

	fb_x11.view_w = view_w;
	fb_x11.view_h = view_h;

#ifdef GFXLIB_NEVERSCALE
	scale = 1;
#else
	scale_x = view_w / fb_x11.content_w;
	scale_y = view_h / fb_x11.content_h;
	scale = (scale_x < scale_y) ? scale_x : scale_y;
	if (scale < 1)
		scale = 1;
#endif

	fb_x11.scale = scale;
	fb_x11.draw_w = fb_x11.content_w * scale;
	fb_x11.draw_h = fb_x11.content_h * scale;
	fb_x11.draw_offset_x = (view_w - fb_x11.draw_w) / 2;
	fb_x11.draw_offset_y = (view_h - fb_x11.draw_h) / 2;
	fb_x11.display_offset = fb_x11.draw_offset_y;
}

static void hWindowToFramebuffer(int window_x, int window_y, int *x, int *y, int *inside)
{
	int rel_x;
	int rel_y;
	int mapped_x;
	int mapped_y;

	if (fb_x11.scale <= 0)
		fb_hX11RefreshLayout(fb_x11.view_w, fb_x11.view_h);

	rel_x = window_x - fb_x11.draw_offset_x;
	rel_y = window_y - fb_x11.draw_offset_y;

	if (inside) {
		*inside = (rel_x >= 0) && (rel_y >= 0) &&
		          (rel_x < fb_x11.draw_w) && (rel_y < fb_x11.draw_h);
	}

	if (rel_x < 0)
		rel_x = 0;
	if (rel_y < 0)
		rel_y = 0;
	if (rel_x >= fb_x11.draw_w)
		rel_x = fb_x11.draw_w - 1;
	if (rel_y >= fb_x11.draw_h)
		rel_y = fb_x11.draw_h - 1;

	mapped_x = rel_x / fb_x11.scale;
	mapped_y = rel_y / fb_x11.scale;

	if (mapped_x < 0)
		mapped_x = 0;
	if (mapped_y < 0)
		mapped_y = 0;
	if (mapped_x >= fb_x11.content_w)
		mapped_x = fb_x11.content_w - 1;
	if (mapped_y >= fb_x11.content_h)
		mapped_y = fb_x11.content_h - 1;

	if (x)
		*x = mapped_x;
	if (y)
		*y = mapped_y;
}

static void hFramebufferToWindow(int x, int y, int *window_x, int *window_y)
{
	if (fb_x11.scale <= 0)
		fb_hX11RefreshLayout(fb_x11.view_w, fb_x11.view_h);

	x = MID(0, x, fb_x11.content_w - 1);
	y = MID(0, y, fb_x11.content_h - 1);

	if (window_x)
		*window_x = fb_x11.draw_offset_x + (x * fb_x11.scale) + (fb_x11.scale / 2);
	if (window_y)
		*window_y = fb_x11.draw_offset_y + (y * fb_x11.scale) + (fb_x11.scale / 2);
}

#if FB_X11_HAS_XINPUT2
static int fb_hX11TouchFind(int source_id, int detail_id)
{
	int i;

	for (i = 0; i < FB_X11_MAX_TOUCH_POINTS; i++) {
		if (touch_points[i].active &&
		    (touch_points[i].source_id == source_id) &&
		    (touch_points[i].detail_id == detail_id))
			return i;
	}
	return -1;
}

static int fb_hX11TouchFindId(int id)
{
	int i;

	for (i = 0; i < FB_X11_MAX_TOUCH_POINTS; i++) {
		if (touch_points[i].active && (touch_points[i].id == id))
			return i;
	}
	return -1;
}

static int fb_hX11TouchAllocate(void)
{
	int i;

	for (i = 0; i < FB_X11_MAX_TOUCH_POINTS; i++) {
		if (!touch_points[i].active)
			return i;
	}
	return -1;
}

static int fb_hX11TouchAllocateId(void)
{
	int attempts;

	for (attempts = 0; attempts <= FB_X11_MAX_TOUCH_POINTS; attempts++) {
		int candidate = touch_next_id;

		touch_next_id = (touch_next_id == INT_MAX) ? 0 : touch_next_id + 1;
		if (fb_hX11TouchFindId(candidate) < 0)
			return candidate;
	}
	return -1;
}

static int fb_hX11TouchCoordinate(double coordinate)
{
	if (!(coordinate > (double)INT_MIN))
		return INT_MIN;
	if (coordinate >= (double)INT_MAX)
		return INT_MAX;
	return (int)coordinate;
}

static void fb_hX11PostTouchMouse(int type, int x, int y)
{
	EVENT event;
	int old_x = mouse_x;
	int old_y = mouse_y;

	fb_hMemSet(&event, 0, sizeof(event));
	event.type = type;
	event.x = x;
	event.y = y;
	event.dx = x - old_x;
	event.dy = y - old_y;
	if ((type == EVENT_MOUSE_BUTTON_PRESS) ||
	    (type == EVENT_MOUSE_BUTTON_RELEASE))
		event.button = BUTTON_LEFT;
	if (__fb_gfx->scanline_size != 1) {
		event.y /= __fb_gfx->scanline_size;
		event.dy /= __fb_gfx->scanline_size;
	}
	mouse_x = x;
	mouse_y = y;
	fb_hPostEvent(&event);
}

static void fb_hX11TouchShutdown(void)
{
	if (touch_api.library)
		dlclose(touch_api.library);
	memset(&touch_api, 0, sizeof(touch_api));
	pthread_mutex_lock(&touch_mutex);
	memset(touch_points, 0, sizeof(touch_points));
	touch_primary_id = -1;
	touch_primary_mouse_down = FALSE;
	touch_next_id = 0;
	pthread_mutex_unlock(&touch_mutex);
}

static int fb_hX11TouchLoad(Display *display)
{
	void *symbol;
	int major = 2;
	int minor = 2;
	int first_event;
	int first_error;

	if (touch_api.library)
		return TRUE;

	touch_api.library = dlopen("libXi.so.6", RTLD_NOW | RTLD_LOCAL);
	if (!touch_api.library)
		touch_api.library = dlopen("libXi.so", RTLD_NOW | RTLD_LOCAL);
	if (!touch_api.library)
		return FALSE;

	symbol = dlsym(touch_api.library, "XIQueryVersion");
	if ((!symbol) || (sizeof(symbol) != sizeof(touch_api.query_version)))
		goto failed;
	memcpy(&touch_api.query_version, &symbol, sizeof(symbol));

	symbol = dlsym(touch_api.library, "XISelectEvents");
	if ((!symbol) || (sizeof(symbol) != sizeof(touch_api.select_events)))
		goto failed;
	memcpy(&touch_api.select_events, &symbol, sizeof(symbol));

	if (!XQueryExtension(display, "XInputExtension", &touch_api.extension_opcode,
			     &first_event, &first_error))
		goto failed;
	if ((touch_api.query_version(display, &major, &minor) != Success) ||
	    (major < 2) || ((major == 2) && (minor < 2)))
		goto failed;

	return TRUE;

failed:
	fb_hX11TouchShutdown();
	return FALSE;
}

static void fb_hX11TouchSelect(Display *display, Window window)
{
	unsigned char mask[XIMaskLen(XI_LASTEVENT)];
	XIEventMask event_mask;

	if ((window == None) || (!fb_hX11TouchLoad(display)))
		return;

	memset(mask, 0, sizeof(mask));
	XISetMask(mask, XI_TouchBegin);
	XISetMask(mask, XI_TouchUpdate);
	XISetMask(mask, XI_TouchEnd);
	event_mask.deviceid = XIAllDevices;
	event_mask.mask_len = sizeof(mask);
	event_mask.mask = mask;
	touch_api.enabled = touch_api.select_events(display, window,
		&event_mask, 1) == Success;
}

static void fb_hX11TouchReset(void)
{
	int release_mouse;

	pthread_mutex_lock(&touch_mutex);
	release_mouse = touch_primary_mouse_down;
	if (release_mouse) {
		mouse_buttons &= ~BUTTON_LEFT;
		mouse_latched_buttons &= ~BUTTON_LEFT;
	}
	memset(touch_points, 0, sizeof(touch_points));
	touch_primary_id = -1;
	touch_primary_mouse_down = FALSE;
	pthread_mutex_unlock(&touch_mutex);
	if (release_mouse)
		fb_hX11PostTouchMouse(EVENT_MOUSE_BUTTON_RELEASE, mouse_x, mouse_y);
}

static void fb_hX11TouchEvent(XEvent *event)
{
	XGenericEventCookie *cookie = &event->xcookie;
	XIDeviceEvent *device_event;
	int slot;
	int inside;
	int id;

	if ((!touch_api.enabled) || (cookie->extension != touch_api.extension_opcode))
		return;
	if ((cookie->evtype != XI_TouchBegin) &&
	    (cookie->evtype != XI_TouchUpdate) &&
	    (cookie->evtype != XI_TouchEnd))
		return;
	if (!XGetEventData(fb_x11.display, cookie))
		return;
	pthread_mutex_lock(&touch_mutex);

	device_event = (XIDeviceEvent *)cookie->data;
	if ((!device_event) || (device_event->event != fb_x11.window))
		goto done;

	if (cookie->evtype == XI_TouchBegin) {
		if (fb_hX11TouchFind(device_event->sourceid, device_event->detail) >= 0)
			goto done;
		slot = fb_hX11TouchAllocate();
		id = fb_hX11TouchAllocateId();
		if ((slot < 0) || (id < 0))
			goto done;
		touch_points[slot].active = TRUE;
		touch_points[slot].id = id;
		touch_points[slot].source_id = device_event->sourceid;
		touch_points[slot].detail_id = device_event->detail;
		hWindowToFramebuffer(fb_hX11TouchCoordinate(device_event->event_x),
			fb_hX11TouchCoordinate(device_event->event_y),
			&touch_points[slot].x, &touch_points[slot].y, &inside);
		if (touch_primary_id < 0) {
			touch_primary_id = id;
			mouse_x_root = fb_hX11TouchCoordinate(device_event->root_x);
			mouse_y_root = fb_hX11TouchCoordinate(device_event->root_y);
			has_focus = TRUE;
			mouse_on = inside;
			if (inside) {
				touch_primary_mouse_down = TRUE;
				mouse_buttons |= BUTTON_LEFT;
				mouse_latched_buttons |= BUTTON_LEFT;
				mouse_on = TRUE;
				fb_hX11PostTouchMouse(EVENT_MOUSE_MOVE,
					touch_points[slot].x, touch_points[slot].y);
				fb_hX11PostTouchMouse(EVENT_MOUSE_BUTTON_PRESS,
					touch_points[slot].x, touch_points[slot].y);
			}
		}
		goto done;
	}

	slot = fb_hX11TouchFind(device_event->sourceid, device_event->detail);
	if (slot < 0)
		goto done;
	hWindowToFramebuffer(fb_hX11TouchCoordinate(device_event->event_x),
			fb_hX11TouchCoordinate(device_event->event_y),
			&touch_points[slot].x, &touch_points[slot].y, &inside);
	id = touch_points[slot].id;
	if (id == touch_primary_id) {
		mouse_x_root = fb_hX11TouchCoordinate(device_event->root_x);
		mouse_y_root = fb_hX11TouchCoordinate(device_event->root_y);
		mouse_on = inside || touch_primary_mouse_down;
		if (touch_primary_mouse_down)
			fb_hX11PostTouchMouse(EVENT_MOUSE_MOVE,
				touch_points[slot].x, touch_points[slot].y);
		if (cookie->evtype == XI_TouchEnd) {
			if (touch_primary_mouse_down) {
				mouse_buttons &= ~BUTTON_LEFT;
				fb_hX11PostTouchMouse(EVENT_MOUSE_BUTTON_RELEASE,
					touch_points[slot].x, touch_points[slot].y);
			}
			touch_primary_mouse_down = FALSE;
			touch_primary_id = -1;
			mouse_on = inside;
		}
	}
	if (cookie->evtype == XI_TouchEnd)
		memset(&touch_points[slot], 0, sizeof(touch_points[slot]));

done:
	pthread_mutex_unlock(&touch_mutex);
	XFreeEventData(fb_x11.display, cookie);
}

int fb_hX11GetTouchCount(void)
{
	int i;
	int count = 0;

	pthread_mutex_lock(&touch_mutex);
	for (i = 0; i < FB_X11_MAX_TOUCH_POINTS; i++)
		if (touch_points[i].active)
			count++;
	pthread_mutex_unlock(&touch_mutex);
	return count;
}

int fb_hX11GetTouch(int index, int *x, int *y, int *id)
{
	int i;
	int seen = 0;

	if (index < 0)
		return -1;
	pthread_mutex_lock(&touch_mutex);
	for (i = 0; i < FB_X11_MAX_TOUCH_POINTS; i++) {
		if (!touch_points[i].active)
			continue;
		if (seen == index) {
			if (x)
				*x = touch_points[i].x;
			if (y)
				*y = touch_points[i].y;
			if (id)
				*id = touch_points[i].id;
			pthread_mutex_unlock(&touch_mutex);
			return 0;
		}
		seen++;
	}
	pthread_mutex_unlock(&touch_mutex);
	return -1;
}

static int fb_hX11GetPrimaryTouch(int *x, int *y)
{
	int slot;

	pthread_mutex_lock(&touch_mutex);
	if (!touch_primary_mouse_down)
		goto not_active;
	slot = fb_hX11TouchFindId(touch_primary_id);
	if (slot < 0)
		goto not_active;
	if (x)
		*x = touch_points[slot].x;
	if (y)
		*y = touch_points[slot].y;
	pthread_mutex_unlock(&touch_mutex);
	return TRUE;

not_active:
	pthread_mutex_unlock(&touch_mutex);
	return FALSE;
}
#else
static void fb_hX11TouchSelect(Display *display, Window window)
{
	(void)display;
	(void)window;
}

static void fb_hX11TouchReset(void)
{
}

static void fb_hX11TouchEvent(XEvent *event)
{
	(void)event;
}

static void fb_hX11TouchShutdown(void)
{
}

int fb_hX11GetTouchCount(void)
{
	return 0;
}

int fb_hX11GetTouch(int index, int *x, int *y, int *id)
{
	(void)index;
	(void)x;
	(void)y;
	(void)id;
	return -1;
}

static int fb_hX11GetPrimaryTouch(int *x, int *y)
{
	(void)x;
	(void)y;
	return FALSE;
}
#endif

static void *window_thread(void *arg)
{
	XEvent event;
	EVENT e;
	int key, old_mouse_x, old_mouse_y;
	uint32_t text_scalar;

	(void)arg;

	is_running = TRUE;
	if (fb_x11.init())
		is_running = FALSE;
	cursor_shown = TRUE;
	mouse_x_root = -1;

	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

	for (;;) {
		fb_hX11Lock();

		/*
		 * The main thread changes is_running while shutting down the driver.
		 * Check it while holding the same mutex used by fb_hX11Exit(), or an
		 * optimized build may keep using a stale value and wait forever in
		 * pthread_join().
		 */
		if (!is_running) {
			fb_hX11Unlock();
			break;
		}

		fb_x11.update();
		for (int drop_index = 0; drop_index < 3; ++drop_index)
			fb_hX11DropTick(&drop_windows[drop_index]);

		/* This line is causing fbgfx OpenGL code to freeze. - GOK
		 * XSync(fb_x11.display, False);
		 */
		while (XPending(fb_x11.display)) {
			int update_mouse_state;
			int lock_mouse_state;

			e.type = 0;
			XNextEvent(fb_x11.display, &event);
			if (fb_hX11DropEvent(&drop_windows[0], &event) ||
			    fb_hX11DropEvent(&drop_windows[1], &event) ||
			    fb_hX11DropEvent(&drop_windows[2], &event))
				continue;
			update_mouse_state =
				(event.type == FocusOut) ||
				(event.type == EnterNotify) ||
				(event.type == LeaveNotify) ||
				(event.type == MotionNotify) ||
				(event.type == ButtonPress) ||
				(event.type == ButtonRelease);
			lock_mouse_state = update_mouse_state ||
				(event.type == FocusIn) ||
				(event.type == MapNotify) ||
				(event.type == GenericEvent) ||
				(event.type == KeyPress) ||
				(event.type == KeyRelease);
			if (lock_mouse_state)
				pthread_mutex_lock(&mouse_state_mutex);
			if (update_mouse_state)
				mouse_state_initialized = TRUE;
			switch (event.type) {
				case FocusIn:
				case MapNotify:
					hSetWindowFocus(TRUE, &e);
					/* fallthrough */

				case Expose:
					fb_hMemSet(__fb_gfx->dirty, TRUE, fb_x11.h);
					break;

				case FocusOut:
					fb_hMemSet(__fb_gfx->key, FALSE, 128);
					fb_hX11TouchReset();
					hSetWindowFocus(FALSE, &e);
					mouse_on = FALSE;
					break;

				case EnterNotify:
					has_focus = TRUE;
					hWindowToFramebuffer(event.xcrossing.x,
					                     event.xcrossing.y, &mouse_x, &mouse_y,
					                     &mouse_on);
					mouse_buttons =
						(event.xcrossing.state & Button1Mask ? BUTTON_LEFT : 0) |
						(event.xcrossing.state & Button3Mask ? BUTTON_RIGHT : 0) |
						(event.xcrossing.state & Button2Mask ? BUTTON_MIDDLE : 0);
					e.type = EVENT_MOUSE_ENTER;
					e.x = mouse_x;
					e.y = mouse_y;
					break;

				case LeaveNotify:
					if (has_focus) {
						mouse_on = FALSE;
						e.type = EVENT_MOUSE_EXIT;
					}
					break;

				case MotionNotify:
					old_mouse_x = mouse_x;
					old_mouse_y = mouse_y;
					mouse_x_root = event.xmotion.x_root;
					mouse_y_root = event.xmotion.y_root;
					hWindowToFramebuffer(event.xmotion.x, event.xmotion.y, &mouse_x, &mouse_y, &mouse_on);
					if (has_focus) {
						e.type = EVENT_MOUSE_MOVE;
						e.x = mouse_x;
						e.y = mouse_y;
						e.dx = mouse_x - old_mouse_x;
						e.dy = mouse_y - old_mouse_y;
						if( __fb_gfx->scanline_size != 1 ) {
							e.y /= __fb_gfx->scanline_size;
							e.dy /= __fb_gfx->scanline_size;
						}
					}
					break;

				case ButtonPress:
					hWindowToFramebuffer(event.xbutton.x, event.xbutton.y, &mouse_x, &mouse_y, &mouse_on);
					mouse_x_root = event.xbutton.x_root;
					mouse_y_root = event.xbutton.y_root;
					has_focus = TRUE;
					switch (event.xbutton.button) {
					case Button1: mouse_buttons |= BUTTON_LEFT; mouse_latched_buttons |= BUTTON_LEFT; e.button = BUTTON_LEFT; break;
					case Button3: mouse_buttons |= BUTTON_RIGHT; mouse_latched_buttons |= BUTTON_RIGHT; e.button = BUTTON_RIGHT; break;
					case Button2: mouse_buttons |= BUTTON_MIDDLE; mouse_latched_buttons |= BUTTON_MIDDLE; e.button = BUTTON_MIDDLE; break;
					case Button4: e.z = mouse_wheel++; break;
					case Button5: e.z = mouse_wheel--; break;
					case Button6: e.w = mouse_hwheel--; break;
					case Button7: e.w = mouse_hwheel++; break;
					}

					switch (event.xbutton.button) {
					case Button4:
					case Button5:
						e.type = EVENT_MOUSE_WHEEL;
						break;
					case Button6:
					case Button7:
						e.type = EVENT_MOUSE_HWHEEL;
						break;
					default:
						e.type = EVENT_MOUSE_BUTTON_PRESS;
						/* Double click check -- done for everything except [h]wheel scrolling */
						if (event.xbutton.time - last_click_time < DOUBLE_CLICK_TIME)
							e.type = EVENT_MOUSE_DOUBLE_CLICK;
						last_click_time = event.xbutton.time;
						break;
					}

					break;

				case ButtonRelease:
					hWindowToFramebuffer(event.xbutton.x, event.xbutton.y, &mouse_x, &mouse_y, &mouse_on);
					e.type = EVENT_MOUSE_BUTTON_RELEASE;
					switch (event.xbutton.button) {
						case Button1:	mouse_buttons &= ~BUTTON_LEFT; e.button = BUTTON_LEFT; break;
						case Button3:	mouse_buttons &= ~BUTTON_RIGHT; e.button = BUTTON_RIGHT; break;
						case Button2:	mouse_buttons &= ~BUTTON_MIDDLE; e.button = BUTTON_MIDDLE; break;
						default:		e.type = 0; break;
					}
					break;

				case ConfigureNotify:
					if (!(fb_x11.flags & DRIVER_FULLSCREEN) &&
					    !(fb_x11.flags & DRIVER_NO_FRAME) &&
					    (event.xconfigure.window == fb_x11.wmwindow)) {
						XResizeWindow(fb_x11.display, fb_x11.window,
						              event.xconfigure.width, event.xconfigure.height);
					}
					if (event.xconfigure.window == fb_x11.window) {
						if (fb_x11.flags & DRIVER_RESIZABLE) {
							int logical_height =
								(event.xconfigure.height +
								 __fb_gfx->scanline_size - 1) /
								__fb_gfx->scanline_size;

							fb_hRequestResize(event.xconfigure.width,
								logical_height);
						} else {
							fb_hX11RefreshLayout(event.xconfigure.width,
								event.xconfigure.height);
						}
						fb_hMemSet(__fb_gfx->dirty, TRUE, fb_x11.h);
						XClearWindow(fb_x11.display, fb_x11.window);
					}
					break;

				case MappingNotify:
					XRefreshKeyboardMapping(&event.xmapping);
					break;

				case KeyPress:
					if (has_focus) {
						e.scancode = fb_x11keycode_to_scancode[event.xkey.keycode];
						e.ascii = 0;
						__fb_gfx->key[e.scancode] = TRUE;

						if( __fb_gfx->key[SC_ENTER] && __fb_gfx->key[SC_ALT] && !(fb_x11.flags & DRIVER_NO_SWITCH) ) {
							hOnAltEnter( );
						} else {
							key = translate_key( &event, e.scancode, &text_scalar );
							if( key )
								fb_hPostKey( key );
							/* Don't return extended keycodes in the ascii field */
							e.ascii = text_scalar ? (int)text_scalar :
								(((key < 0) || (key > 0xFF)) ? 0 : key);
						}

						e.type = EVENT_KEY_PRESS;
					}
					break;

				case KeyRelease:
					if (has_focus) {
						e.scancode = fb_x11keycode_to_scancode[event.xkey.keycode];
						key = translate_key( &event, e.scancode, &text_scalar );
						/* Don't return extended keycodes in the ascii field */
						e.ascii = text_scalar ? (int)text_scalar :
							(((key < 0) || (key > 0xFF)) ? 0 : key);
						if (key_repeated(&event)) {
							if( key )
								fb_hPostKey( key );
							e.type = EVENT_KEY_REPEAT;
						} else {
							__fb_gfx->key[e.scancode] = FALSE;
							e.type = EVENT_KEY_RELEASE;
						}
					}
					break;

				case ClientMessage:
					if ((Atom)event.xclient.data.l[0] == wm_delete_window) {
						fb_hPostKey( KEY_QUIT );
						e.type = EVENT_WINDOW_CLOSE;
					}
					break;

				case GenericEvent:
					fb_hX11TouchEvent(&event);
					break;
			}
			if (lock_mouse_state)
				pthread_mutex_unlock(&mouse_state_mutex);
			if (e.type)
				fb_hPostEvent(&e);
		}

		pthread_cond_signal(&cond);

		fb_hX11Unlock();

		usleep(30000);
	}

	for (int drop_index = 0; drop_index < 3; ++drop_index)
		if (drop_windows[drop_index].waiting)
			fb_hX11DropFinish(&drop_windows[drop_index], FALSE);
	fb_x11.exit();

	return NULL;
}

int fb_hX11EnterFullscreen(int *h)
{
	if ((!fb_x11.config) || (target_size < 0))
		return -1;

	if (target_rate < 0) {
		if (XRRSetScreenConfig(fb_x11.display, fb_x11.config, root_window, target_size, orig_rotation, CurrentTime) == BadValue)
			return -1;
	} else {
		if (XRRSetScreenConfigAndRate(fb_x11.display, fb_x11.config, root_window, target_size, orig_rotation, target_rate, CurrentTime) == BadValue)
			return -1;
		__fb_gfx->refresh_rate = fb_x11.refresh_rate = target_rate;
	}

	XWarpPointer(fb_x11.display, None, fb_x11.window, 0, 0, 0, 0, fb_x11.w >> 1, real_h >> 1);
	XSync(fb_x11.display, False);
	while (XGrabPointer(fb_x11.display, fb_x11.window, True, 0,
			    GrabModeAsync, GrabModeAsync, fb_x11.window, None, CurrentTime) != GrabSuccess)
		usleep(10000);
	/*
		Do not grab the keyboard for fullscreen windows.  The window manager
		must retain control of global shortcuts such as Alt+Tab; a keyboard
		grab here prevents the user from switching away from an OpenGL
		fullscreen program.  The fullscreen window receives normal keyboard
		events while it owns input focus.
	*/

	current_size = target_size;
	*h = real_h;

	return 0;
}

void fb_hX11LeaveFullscreen(void)
{
	if ((!fb_x11.config) || (target_size < 0))
		return;

	if (current_size != orig_size) {
		if ((target_rate <= 0) || (XRRSetScreenConfigAndRate(fb_x11.display, fb_x11.config, root_window, orig_size, orig_rotation, orig_rate, CurrentTime) == BadValue))
			XRRSetScreenConfig(fb_x11.display, fb_x11.config, root_window, orig_size, orig_rotation, CurrentTime);
		XUngrabPointer(fb_x11.display, CurrentTime);
		current_size = orig_size;
		__fb_gfx->refresh_rate = fb_x11.refresh_rate = orig_rate;
	}
}

static Bool hMappedEvent(Display *display, XEvent *event, XPointer target)
{
	(void)display;
	return event->type == MapNotify && event->xmap.event == *(Window *)target;
}

void WaitMapped(Window w)
{
	XEvent e;
	/* Hildon can configure the usable client size before mapping finishes.
	 * Leave that event queued so the window loop resizes the drawing child
	 * and framebuffer, instead of retaining the larger startup geometry. */
	XIfEvent(fb_x11.display, &e, hMappedEvent, (XPointer)&w);
}

void fb_hX11InitWindow(int x, int y)
{
	XEvent event;

	if (!(fb_x11.flags & DRIVER_FULLSCREEN)){
		/* windowed */
		fb_hX11RefreshLayout(fb_x11.view_w, fb_x11.view_h);
		XResizeWindow(fb_x11.display, fb_x11.wmwindow, fb_x11.view_w, fb_x11.view_h);
		XResizeWindow(fb_x11.display, fb_x11.window, fb_x11.view_w, fb_x11.view_h);

		if (!(fb_x11.flags & DRIVER_NO_FRAME)) {
			XReparentWindow(fb_x11.display, fb_x11.window, fb_x11.wmwindow, 0, 0);
			XMapRaised(fb_x11.display,fb_x11.wmwindow);
			WaitMapped(fb_x11.wmwindow);
		}
		XMapRaised(fb_x11.display, fb_x11.window);
		if (fb_x11.flags & DRIVER_NO_FRAME)
			XMoveWindow(fb_x11.display, fb_x11.window, x, y);
		WaitMapped(fb_x11.window);
		XRaiseWindow(fb_x11.display, fb_x11.window);
	} else {
		/* fullscreen */
		fb_hX11RefreshLayout(fb_x11.view_w, fb_x11.view_h);
		XMoveResizeWindow(fb_x11.display, fb_x11.fswindow, 0, 0, fb_x11.view_w, fb_x11.view_h);
		XMoveResizeWindow(fb_x11.display, fb_x11.window, 0, 0, fb_x11.view_w, fb_x11.view_h);
		XReparentWindow(fb_x11.display, fb_x11.window, fb_x11.fswindow, 0, 0);
		XMapRaised(fb_x11.display, fb_x11.fswindow);
		/* use XSync instead of WaitMapped for unmanaged windows */
		XSync(fb_x11.display, False);
		XMapRaised(fb_x11.display, fb_x11.window);
		XSync(fb_x11.display, False);
		XRaiseWindow(fb_x11.display, fb_x11.window);
		/*
			The fullscreen parent is override-redirect, so the window manager
			does not select it as the active window after mapping.  Assign focus
			once to the newly mapped drawing child.  This is deliberately not a
			keyboard grab: subsequent Alt+Tab focus changes remain under window
			manager control.
		*/
		XSetInputFocus(fb_x11.display, fb_x11.window, RevertToParent,
			CurrentTime);
	}

	if (fb_x11.flags & DRIVER_ALWAYS_ON_TOP) {
		fb_hMemSet(&event, 0, sizeof(event));
		event.xclient.type = ClientMessage;
		event.xclient.send_event = True;
		event.xclient.message_type = XInternAtom(fb_x11.display, "_NET_WM_STATE", False);
		event.xclient.window = fb_x11.wmwindow;
		event.xclient.format = 32;
		event.xclient.data.l[0] = 1;
		event.xclient.data.l[1] = XInternAtom(fb_x11.display, "_NET_WM_STATE_ABOVE", False);
		XSendEvent(fb_x11.display, root_window, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
	}
}

void fb_hXlibInit(void)
{
	if (!xlib_inited) {
		XInitThreads();
		xlib_inited = TRUE;
	}
}

int fb_hX11Init(char *title, int w, int h, int depth, int refresh_rate, int flags)
{
	XPixmapFormatValues *format;
	XSetWindowAttributes attribs;
	XWMHints hints;
#ifndef DISABLE_XPM
	XpmAttributes xpm_attribs;
#endif
	XSizeHints *size;
	Pixmap pixmap;
	XColor color;
	XGCValues gc_values;
	XRRScreenSize *sizes;
	short *rates;
	int version, dummy;
	int i, num_formats, num_sizes, num_rates, supersized_h;
	int gc_mask;
	const char *intern_atoms[] = { "_MOTIF_WM_HINTS", "KWM_WIN_DECORATION", "_WIN_HINTS" };
	int intern_hints[] = { 0x2, 0, 0, 0, 0 };

	is_running = FALSE;
	fb_hXlibInit();

	fb_x11.w = w;
	fb_x11.h = h;
	fb_x11.content_w = __fb_gfx ? __fb_gfx->w : w;
	fb_x11.content_h = __fb_gfx ? (__fb_gfx->h * __fb_gfx->scanline_size) : h;
	fb_x11.view_w = w;
	fb_x11.view_h = h;
	fb_hX11RefreshLayout(w, h);
	fb_x11.flags = flags;
	fb_x11.refresh_rate = refresh_rate;

	target_size = -1;
	target_rate = -1;
	current_size = -1;
	supersized_h = calc_comp_height(fb_x11.h);

	color_map = None;
	arrow_cursor = None;
	wm_intern_hints = None;

	if (fb_x11.visual) {
		fb_x11.depth = depth;
	} else {
		fb_x11.display = XOpenDisplay(NULL);
		if (!fb_x11.display)
			return -1;
		fb_x11.screen = XDefaultScreen(fb_x11.display);
		fb_x11.visual = XDefaultVisual(fb_x11.display, fb_x11.screen);
		fb_x11.depth = XDefaultDepth(fb_x11.display, fb_x11.screen);
		format = XListPixmapFormats(fb_x11.display, &num_formats);
		for (i = 0; i < num_formats; i++) {
			if (format[i].depth == fb_x11.depth) {
				if (format[i].bits_per_pixel == 16)
					fb_x11.visual_depth = format[i].depth;
				else
					fb_x11.visual_depth = format[i].bits_per_pixel;
				break;
			}
		}
		XFree(format);
	}
	root_window = XDefaultRootWindow(fb_x11.display);

	attribs.border_pixel = attribs.background_pixel = XBlackPixel(fb_x11.display, fb_x11.screen);
	attribs.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
			     PointerMotionMask | FocusChangeMask | EnterWindowMask | LeaveWindowMask | ExposureMask | StructureNotifyMask;
	attribs.backing_store = NotUseful;
	attribs.colormap = XCreateColormap( fb_x11.display, root_window, fb_x11.visual, AllocNone);
	fb_x11.window = XCreateWindow(fb_x11.display, root_window, 0, 0, fb_x11.w, fb_x11.h,
					0, fb_x11.depth, InputOutput, fb_x11.visual,
					CWBackPixel | CWBorderPixel | CWEventMask | CWBackingStore | CWColormap, &attribs);
	fb_x11.wmwindow = XCreateWindow(fb_x11.display, root_window, 0, 0, fb_x11.w, fb_x11.h,
					0, fb_x11.depth, InputOutput, fb_x11.visual,
					CWBackPixel | CWBorderPixel | CWEventMask | CWBackingStore | CWColormap, &attribs);
	attribs.override_redirect = True;
	fb_x11.fswindow = XCreateWindow(fb_x11.display, root_window, 0, 0, fb_x11.w, fb_x11.h,
					0, fb_x11.depth, InputOutput, fb_x11.visual,
					CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask | CWBackingStore | CWColormap, &attribs);

	if (!fb_x11.window)
		return -1;
	fb_hX11TouchSelect(fb_x11.display, fb_x11.window);
	fb_hX11DropInit(&drop_windows[0], fb_x11.display, fb_x11.window,
		x11_drop_enabled, x11_drop_submit, NULL);
	fb_hX11DropInit(&drop_windows[1], fb_x11.display, fb_x11.wmwindow,
		x11_drop_enabled, x11_drop_submit, NULL);
	fb_hX11DropInit(&drop_windows[2], fb_x11.display, fb_x11.fswindow,
		x11_drop_enabled, x11_drop_submit, NULL);
	EVENT_LOCK();
	__fb_gfx->drops.supported = TRUE;
	EVENT_UNLOCK();
	fb_hX11SetWindowTitle( title );
#ifndef DISABLE_XPM
	if (fb_program_icon) {
		Window icon_window = (flags & DRIVER_NO_FRAME) ? fb_x11.window : fb_x11.wmwindow;

		hints.flags = IconPixmapHint | IconMaskHint;
		xpm_attribs.valuemask = XpmReturnAllocPixels | XpmReturnExtensions;
		XpmCreatePixmapFromData(fb_x11.display, icon_window, fb_program_icon, &hints.icon_pixmap, &hints.icon_mask, &xpm_attribs);
		XSetWMHints(fb_x11.display, icon_window, &hints);
	}
#endif

	size = XAllocSizeHints();
	size->flags = PBaseSize | PMinSize | PResizeInc;
	/* The display size at startup is not a stable limit after RandR rotation. */
	if (!(flags & DRIVER_RESIZABLE))
		size->flags |= PMaxSize;
	size->base_width = fb_x11.content_w;
	size->base_height = fb_x11.content_h;
	if (flags & DRIVER_RESIZABLE) {
		size->min_width = 8;
		size->min_height = (__fb_gfx && __fb_gfx->font) ?
			__fb_gfx->font->h * __fb_gfx->scanline_size : 16;
	} else {
		size->min_width = fb_x11.content_w;
		size->min_height = fb_x11.content_h;
	}
	if ((flags & DRIVER_NO_SWITCH) && !(flags & DRIVER_RESIZABLE)) {
		size->max_width = size->min_width;
		size->max_height = size->min_height;
	} else if (!(flags & DRIVER_RESIZABLE)) {
		size->max_width = XDisplayWidth(fb_x11.display, fb_x11.screen);
		size->max_height = XDisplayHeight(fb_x11.display, fb_x11.screen);
	}
	size->width_inc = ((flags & DRIVER_NO_SWITCH) &&
		!(flags & DRIVER_RESIZABLE)) ? 0x10000 : 1;
	size->height_inc = ((flags & DRIVER_NO_SWITCH) &&
		!(flags & DRIVER_RESIZABLE)) ? 0x10000 : 1;
	XSetWMNormalHints(fb_x11.display, fb_x11.window, size);
	XSetWMNormalHints(fb_x11.display, fb_x11.fswindow, size);
	XSetWMNormalHints(fb_x11.display, fb_x11.wmwindow, size);
	XFree(size);
	/* Hildon keeps clients in a landscape surface unless they advertise
	 * portrait support. Resize-capable windows can follow its portrait
	 * configure events; fixed framebuffer windows keep their existing policy.
	 * Only set this extension when its atom is already known to the server. */
	if (flags & DRIVER_RESIZABLE) {
		Atom portrait = XInternAtom(fb_x11.display,
			"_HILDON_PORTRAIT_MODE_SUPPORT", True);
		if (portrait != None) {
			unsigned long supported = 1;
			Window managed = (flags & DRIVER_NO_FRAME) ?
				fb_x11.window : fb_x11.wmwindow;
			XChangeProperty(fb_x11.display, managed, portrait, XA_CARDINAL,
				32, PropModeReplace, (unsigned char *)&supported, 1);
		}
	}

	if (flags & DRIVER_NO_FRAME) {
		for (i = 0; i < 3; i++) {
			wm_intern_hints = XInternAtom(fb_x11.display, intern_atoms[i], True);
			if (wm_intern_hints != None) {
				XChangeProperty(fb_x11.display, fb_x11.window, wm_intern_hints, wm_intern_hints,
					32, PropModeReplace, (unsigned char *)&intern_hints[i], (i == 0) ? 5 : 1);
				break;
			}
		}
		if (wm_intern_hints == None)
			return -1;
	}

	wm_delete_window = XInternAtom(fb_x11.display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(fb_x11.display, fb_x11.wmwindow, &wm_delete_window, 1);

	if (fb_x11.visual->class == PseudoColor) {
		color_map = XCreateColormap(fb_x11.display, root_window, fb_x11.visual, AllocAll);
		XSetWindowColormap(fb_x11.display, fb_x11.window, color_map);
	}
	XClearWindow(fb_x11.display, fb_x11.window);

	pixmap = XCreatePixmap(fb_x11.display, fb_x11.window, 1, 1, 1);
	gc_mask = GCFunction | GCForeground | GCBackground;
	gc_values.function = GXcopy;
	gc_values.foreground = gc_values.background = 0;
	fb_x11.gc = XCreateGC(fb_x11.display, pixmap, gc_mask, &gc_values);
	XDrawPoint(fb_x11.display, pixmap, fb_x11.gc, 0, 0);
	XFreeGC(fb_x11.display, fb_x11.gc);
	color.pixel = color.red = color.green = color.blue = 0;
	color.flags = DoRed | DoGreen | DoBlue;
	blank_cursor = XCreatePixmapCursor(fb_x11.display, pixmap, pixmap, &color, &color, 0, 0);
	arrow_cursor = XCreateFontCursor(fb_x11.display, XC_left_ptr);
	XFreePixmap(fb_x11.display, pixmap);
	fb_x11.gc = DefaultGC(fb_x11.display, fb_x11.screen);
	XSync(fb_x11.display, False);

	if (XRRQueryExtension(fb_x11.display, &dummy, &dummy) &&
	    XRRQueryVersion(fb_x11.display, &version, &dummy) && (version >= 1)) {
		fb_x11.config = XRRGetScreenInfo(fb_x11.display, root_window);
		orig_size = current_size = XRRConfigCurrentConfiguration(fb_x11.config, &orig_rotation);
		orig_rate = XRRConfigCurrentRate(fb_x11.config);
		sizes = XRRConfigSizes(fb_x11.config, &num_sizes);
		for (i = 0; i < num_sizes; i++) {
			if (sizes[i].width == fb_x11.w) {
				if (sizes[i].height == fb_x11.h) {
					target_size = i;
					real_h = fb_x11.h;
					break;
				}
				else if (sizes[i].height == supersized_h) {
					target_size = i;
					real_h = supersized_h;
					break;
				}
			}
		}
		if ((fb_x11.refresh_rate > 0) && (target_size >= 0)) {
			rates = XRRConfigRates(fb_x11.config, target_size, &num_rates);
			for (i = 0; i < num_rates; i++) {
				if (rates[i] == fb_x11.refresh_rate) {
					target_rate = rates[i];
					break;
				}
			}
		} else {
			fb_x11.refresh_rate = orig_rate;
		}
		__fb_gfx->refresh_rate = fb_x11.refresh_rate;
	}

	fb_hInitX11KeycodeToScancodeTb( fb_x11.display, XDisplayKeycodes, XGetKeyboardMapping, XFree );

	if (flags & DRIVER_FULLSCREEN) {
		has_focus = TRUE;
		mouse_on = TRUE;
		mouse_state_initialized = TRUE;
		mouse_x = fb_x11.content_w >> 1;
		mouse_y = fb_x11.content_h >> 1;
	} else {
		has_focus = FALSE;
		mouse_on = FALSE;
		mouse_state_initialized = FALSE;
	}
	focus_notified = has_focus;
	mouse_buttons = mouse_latched_buttons = mouse_wheel = 0;
	fb_hX11TouchReset();

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	pthread_mutex_lock(&mutex);
	if (!pthread_create(&thread, NULL, window_thread, NULL)) {
		pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);
		if (is_running)
			if (__fb_gfx)
				__fb_gfx->get_mouse_fast = fb_hX11GetMouseFast;
		if (is_running)
			return 0;
		pthread_join(thread, NULL);
	}
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);

	return -1;
}

void fb_hX11Exit(void)
{
#ifdef HOST_LINUX
	/* Stop input callbacks before the X11 lock and mode storage are torn down. */
	fb_hLinuxMaliitCancelPending();
#endif
	if (__fb_gfx)
		__fb_gfx->get_mouse_fast = NULL;
	if (is_running) {
		pthread_mutex_lock(&mutex);
		is_running = FALSE;
		pthread_mutex_unlock(&mutex);
		pthread_join(thread, NULL);
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&cond);
	}
	if (fb_x11.display) {
		fb_hX11LeaveFullscreen();
		XSync(fb_x11.display, False);
		if (arrow_cursor != None) {
			XUndefineCursor(fb_x11.display, fb_x11.window);
			XFreeCursor(fb_x11.display, arrow_cursor);
			arrow_cursor = None;
			XFreeCursor(fb_x11.display, blank_cursor);
			blank_cursor = None;
		}
		if (color_map != None) {
			XFreeColormap(fb_x11.display, color_map);
			color_map = None;
		}
		if (wm_intern_hints != None) {
			XDeleteProperty(fb_x11.display, fb_x11.window, wm_intern_hints);
			wm_intern_hints = None;
		}
		if (fb_x11.window != None) {
			XDestroyWindow(fb_x11.display, fb_x11.window);
			fb_x11.window = None;
		}
		if (fb_x11.fswindow != None) {
			XDestroyWindow(fb_x11.display, fb_x11.fswindow);
			fb_x11.fswindow = None;
		}
		if (fb_x11.wmwindow != None) {
			XDestroyWindow(fb_x11.display, fb_x11.wmwindow);
			fb_x11.wmwindow = None;
		}
		if (fb_x11.config) {
			XRRFreeScreenConfigInfo(fb_x11.config);
			fb_x11.config = NULL;
		}
		XCloseDisplay(fb_x11.display);
		fb_x11.display = NULL;
	}
	fb_hX11TouchShutdown();
}

void fb_hX11Lock(void)
{
	pthread_mutex_lock(&mutex);
	XLockDisplay(fb_x11.display);
}

void fb_hX11Unlock(void)
{
	XUnlockDisplay(fb_x11.display);
	pthread_mutex_unlock(&mutex);
}

int fb_hX11WindowHasFocus(void)
{
	int focused;
	fb_hX11Lock();
	focused = focus_notified;
	fb_hX11Unlock();
	return focused;
}

void fb_hX11SetPalette(int index, int r, int g, int b)
{
	XColor color;

	if (fb_x11.visual->class == PseudoColor) {
		color.pixel = index;
		color.red = (r << 8) | r;
		color.green = (g << 8) | g;
		color.blue = (b << 8) | b;
		color.flags = DoRed | DoGreen | DoBlue;
		XStoreColors(fb_x11.display, color_map, &color, 1);
	}
}

void fb_hX11WaitVSync(void)
{
	usleep(1000000 / ((fb_x11.refresh_rate > 0) ? fb_x11.refresh_rate : 60));
}

int fb_hX11GetMouseFast(int *x, int *y, int *z, int *buttons, int *clip)
{
	int available;

	pthread_mutex_lock(&mouse_state_mutex);
	if (mouse_on) {
		available = 1;
	} else if ((!mouse_state_initialized) || mouse_buttons || mouse_latched_buttons) {
		available = 0;
	} else {
		available = -1;
	}
	if (available > 0) {
		if (x) *x = mouse_x;
		if (y) *y = mouse_y;
		if (z) *z = mouse_wheel;
		if (buttons) {
			*buttons = mouse_buttons | mouse_latched_buttons;
			mouse_latched_buttons = 0;
		}
		if (clip) *clip = fb_x11.mouse_clip;
	}
	pthread_mutex_unlock(&mouse_state_mutex);

	return available;
}

int fb_hX11GetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	Window root, child;
	int root_x, root_y, win_x, win_y;
	int query_x, query_y, query_inside;
	int touch_x, touch_y;
	unsigned int buttons_mask;
	const int invalid_coord = -1;
	int cached_inside;
	int cached_clip;

	if (x)
		*x = invalid_coord;
	if (y)
		*y = invalid_coord;
	if (buttons)
		*buttons = 0;

	pthread_mutex_lock(&mouse_state_mutex);
	cached_inside = mouse_on;
	cached_clip = fb_x11.mouse_clip;
	if (z) *z = mouse_wheel;
	if (fb_hX11GetPrimaryTouch(&touch_x, &touch_y)) {
		if (x)
			*x = touch_x;
		if (y)
			*y = touch_y;
		if (buttons)
			*buttons = mouse_buttons | mouse_latched_buttons | BUTTON_LEFT;
		if (clip)
			*clip = fb_x11.mouse_clip;
		if (buttons)
			mouse_latched_buttons = 0;
		pthread_mutex_unlock(&mouse_state_mutex);
		return 0;
	}
	/* Keep a short-press visible once even if polling sees pointer outside. */
	if ((!mouse_on) && (!mouse_latched_buttons) && (buttons == NULL)) {
		if (clip) *clip = cached_clip;
		pthread_mutex_unlock(&mouse_state_mutex);
		return 0;
	}
	/*
		MotionNotify and button events already keep this state current while the
		pointer is inside the window. Reuse it instead of synchronously asking
		the X server for the same position on every GETMOUSE call. A pointer
		dragged outside the window still uses XQueryPointer below.
	*/
	if (cached_inside) {
		if (x) *x = mouse_x;
		if (y) *y = mouse_y;
		if (buttons) {
			*buttons = mouse_buttons | mouse_latched_buttons;
			mouse_latched_buttons = 0;
		}
		if (clip) *clip = cached_clip;
		pthread_mutex_unlock(&mouse_state_mutex);
		return 0;
	}
	pthread_mutex_unlock(&mouse_state_mutex);

	if (XQueryPointer(fb_x11.display, fb_x11.window, &root, &child, &root_x, &root_y, &win_x, &win_y, &buttons_mask)) {
		query_x = win_x;
		query_y = win_y;
		query_inside = FALSE;
		hWindowToFramebuffer(win_x, win_y, &query_x, &query_y, &query_inside);
		/*
			When a window is mapped underneath a stationary pointer, some window
			managers omit the initial EnterNotify. Seed the event cache from this
			one synchronous query so later GETMOUSE calls can use the fast path.
		*/
		pthread_mutex_lock(&mouse_state_mutex);
		mouse_x = query_x;
		mouse_y = query_y;
		mouse_on = query_inside;
		mouse_state_initialized = TRUE;
		mouse_buttons = (buttons_mask & Button1Mask ? 0x1 : 0) |
				(buttons_mask & Button3Mask ? 0x2 : 0) |
				(buttons_mask & Button2Mask ? 0x4 : 0);
		if (query_inside)
			has_focus = TRUE;
		mouse_x_root = root_x;
		mouse_y_root = root_y;
		if (x) *x = mouse_x;
		if (y) *y = mouse_y;
		if (buttons) {
			*buttons = mouse_buttons | mouse_latched_buttons;
			mouse_latched_buttons = 0;
		}
		cached_clip = fb_x11.mouse_clip;
		pthread_mutex_unlock(&mouse_state_mutex);
	} else {
		pthread_mutex_lock(&mouse_state_mutex);
		if (x) *x = mouse_x;
		if (y) *y = mouse_y;
		if (buttons) {
			*buttons = mouse_buttons | mouse_latched_buttons;
			mouse_latched_buttons = 0;
		}
		cached_clip = fb_x11.mouse_clip;
		pthread_mutex_unlock(&mouse_state_mutex);
	}

	if (clip) *clip = cached_clip;
	return 0;
}

void fb_hX11SetMouse(int x, int y, int show, int clip)
{
	int warp_x = 0, warp_y = 0, warp_pointer = FALSE;

	pthread_mutex_lock(&mouse_state_mutex);
	if ((x != (int)0x80000000 || y != (int)0x80000000) && has_focus) {
		if (x == (int)0x80000000) {
			x = mouse_x;
		}
		else if (y == (int)0x80000000) {
			y = mouse_y;
		}

		x = MID(0, x, fb_x11.content_w - 1);
		y = MID(0, y, fb_x11.content_h - 1);

		mouse_on = TRUE;
		mouse_x = x;
		mouse_y = y;
		mouse_state_initialized = TRUE;
		warp_x = mouse_x;
		warp_y = mouse_y;
		warp_pointer = TRUE;
	}
	if (clip == 0)
		fb_x11.mouse_clip = FALSE;
	else if (clip > 0)
		fb_x11.mouse_clip = TRUE;
	pthread_mutex_unlock(&mouse_state_mutex);

	if (warp_pointer) {
		x = warp_x;
		y = warp_y;
		hFramebufferToWindow(x, y, &x, &y);
		XWarpPointer(fb_x11.display, None, fb_x11.window, 0, 0, 0, 0, x, y);
	}
	if ((show > 0) && (!cursor_shown)) {
		XUndefineCursor(fb_x11.display, fb_x11.window);
		XDefineCursor(fb_x11.display, fb_x11.window, arrow_cursor);
		cursor_shown = TRUE;
	}
	else if ((show == 0) && (cursor_shown)) {
		XUndefineCursor(fb_x11.display, fb_x11.window);
		XDefineCursor(fb_x11.display, fb_x11.window, blank_cursor);
		cursor_shown = FALSE;
	}
	if (clip == 0) {
		XUngrabPointer(fb_x11.display, CurrentTime);
	}
	else if (clip > 0) {
		while (1) {
			if (XGrabPointer(fb_x11.display, fb_x11.window, True, 0, GrabModeAsync, GrabModeAsync, fb_x11.window, None, CurrentTime) == GrabSuccess)
				break;
			usleep(100000);
		}
	}
}

void fb_hX11SetWindowTitle(char *title)
{
	if (fb_x11.flags & DRIVER_NO_FRAME) {
		XStoreName(fb_x11.display, fb_x11.window, title);
	} else {
		XStoreName(fb_x11.display, fb_x11.wmwindow, title);
	}
}

int fb_hX11SetWindowPos(int x, int y)
{
	Window window, root, parent, *children;
	XWindowAttributes attribs = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	XEvent event;
	unsigned int num_children;
	int dx = 0, dy = 0;

	if (fb_x11.flags & DRIVER_FULLSCREEN)
		return 0;
	fb_hX11Lock();
	parent = fb_x11.window;
	do {
		window = parent;
		dx += attribs.x;
		dy += attribs.y;
		XGetWindowAttributes(fb_x11.display, window, &attribs);
		XQueryTree(fb_x11.display, window, &root, &parent, &children, &num_children);
		if (children) XFree(children);
	} while (parent != root_window);
	if (x == (int)0x80000000)
		x = attribs.x;
	else
		x -= dx;
	if (y == (int)0x80000000)
		y = attribs.y;
	else
		y -= dy;

	if (fb_x11.flags & DRIVER_NO_FRAME)
		XMoveWindow(fb_x11.display, fb_x11.window, x, y);
	else
		XMoveWindow(fb_x11.display, fb_x11.wmwindow, x, y);

	XClearWindow(fb_x11.display, fb_x11.wmwindow);

	/* remove any mouse motion events */
	while (XCheckWindowEvent(fb_x11.display, fb_x11.window, PointerMotionMask, &event))
		;
	fb_hX11Unlock();

	return ((attribs.x + dx) & 0xFFFF) | ((attribs.y + dy) << 16);
}

int fb_hX11GetGLViewport(int *x, int *y, int *w, int *h)
{
	if (!fb_x11.display)
		return 0;

	fb_hX11RefreshLayout(fb_x11.view_w, fb_x11.view_h);
	if ((fb_x11.draw_w <= 0) || (fb_x11.draw_h <= 0))
		return 0;

	if (x)
		*x = fb_x11.draw_offset_x;
	if (y)
		*y = fb_x11.view_h - (fb_x11.draw_offset_y + fb_x11.draw_h);
	if (w)
		*w = fb_x11.draw_w;
	if (h)
		*h = fb_x11.draw_h;

	return 1;
}

int *fb_hX11FetchModes(int depth, int *size)
{
	Display *dpy;
	XRRScreenConfiguration *cfg;
	XRRScreenSize *rr_sizes;
	int i, *sizes = NULL;

	if ((depth != 8) && (depth != 15) && (depth != 16) && (depth != 24) && (depth != 32))
		return NULL;

	if (fb_x11.display)
		dpy = fb_x11.display;
	else
		dpy = XOpenDisplay(NULL);
	if (!dpy)
		return NULL;

	if (fb_x11.config)
		cfg = fb_x11.config;
	else
		cfg = XRRGetScreenInfo(dpy, XDefaultRootWindow(dpy));
	if (!cfg)
		return NULL;

	rr_sizes = XRRConfigSizes(cfg, size);
	if ((rr_sizes) && (*size > 0)) {
		sizes = (int *)malloc(*size * sizeof(int));
		if (sizes) {
			for (i = 0; i < *size; i++)
				sizes[i] = (rr_sizes[i].width << 16) | (rr_sizes[i].height);
		} else {
			*size = 0;
		}
	}
	if (!fb_x11.config)
		XRRFreeScreenConfigInfo(cfg);
	if (!fb_x11.display)
		XCloseDisplay(dpy);

	return sizes;
}

int fb_hX11ScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	XRRScreenConfiguration *cfg;
	Display *dpy;
	int dummy, version;

	dpy = XOpenDisplay(NULL);
	if (!dpy)
		return -1;

	*width = XDisplayWidth(dpy, XDefaultScreen(dpy));
	*height = XDisplayHeight(dpy, XDefaultScreen(dpy));
	*depth = XDefaultDepth(dpy, XDefaultScreen(dpy));
	if (XRRQueryExtension(dpy, &dummy, &dummy) &&
	    XRRQueryVersion(dpy, &version, &dummy) && (version >= 1)) {
		/* XRRGetScreenInfo() will fail if RandR extension isn't available */
		cfg = XRRGetScreenInfo(dpy, XDefaultRootWindow(dpy));
		if( cfg ) {
			*refresh = XRRConfigCurrentRate(cfg);
			XRRFreeScreenConfigInfo(cfg);
		} else {
			*refresh = 0;
		}
	} else {
		*refresh = 0;
	}

	XCloseDisplay(dpy);

	return 0;
}

ssize_t fb_hGetWindowHandle(void)
{
	return (fb_x11.display ? fb_x11.window : 0);
}

ssize_t fb_hGetDisplayHandle(void)
{
	/* SCREENCONTROL exposes platform handles through integer-sized slots. */
	/* cppcheck-suppress CastAddressToIntegerAtReturn */
	return (ssize_t)(intptr_t)fb_x11.display;
}

#else

ssize_t fb_hGetWindowHandle(void)
{
	return 0;
}

ssize_t fb_hGetDisplayHandle(void)
{
	return 0;
}

#endif
/* end of gfx_x11.c */
