/* x11 window management code shared by x11 and opengl drivers */

#include <stdint.h>
#include <sys/types.h>

#ifndef DISABLE_X11

#include "../fb_gfx.h"
#include "fb_gfx_x11.h"
#include "../../rtlib/unix/fb_private_scancodes_x11.h"
#include <pthread.h>

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
/* Keep a one-shot latch of press events in case polling misses a short click. */
static int mouse_x, mouse_y, mouse_wheel, mouse_hwheel, mouse_buttons, mouse_latched_buttons, mouse_on;
static int mouse_x_root, mouse_y_root;

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

static int translate_key(XEvent *event, int scancode)
{
	unsigned char key[8];
	int k;

	if( XLookupString( &event->xkey, (char *)key, 8, NULL, NULL ) == 1 ) {
		k = key[0];

		/* Remap ASCII DEL to FB's extended keycode for DELETE,
		   to match behaviour of console mode and other platforms */
		if( k == 0x7F ) {
			k = KEY_DEL;
		}
	} else {
		k = fb_hScancodeToExtendedKey( scancode );
	}

	return k;
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

static void *window_thread(void *arg)
{
	XEvent event;
	EVENT e;
	int key, old_mouse_x, old_mouse_y;

	(void)arg;

	is_running = TRUE;
	if (fb_x11.init())
		is_running = FALSE;
	cursor_shown = TRUE;
	mouse_x_root = -1;

	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

	while (is_running) {
		fb_hX11Lock();

		fb_x11.update();

		/* This line is causing fbgfx OpenGL code to freeze. - GOK
		 * XSync(fb_x11.display, False);
		 */
		while (XPending(fb_x11.display)) {
			e.type = 0;
			XNextEvent(fb_x11.display, &event);
			switch (event.type) {
				case FocusIn:
				case MapNotify:
					if (!has_focus) {
						has_focus = TRUE;
						e.type = EVENT_WINDOW_GOT_FOCUS;
					}
					/* fallthrough */

				case Expose:
					fb_hMemSet(__fb_gfx->dirty, TRUE, fb_x11.h);
					break;

				case FocusOut:
					fb_hMemSet(__fb_gfx->key, FALSE, 128);
					has_focus = mouse_on = FALSE;
					e.type = EVENT_WINDOW_LOST_FOCUS;
					break;

				case EnterNotify:
					has_focus = TRUE;
					mouse_on = TRUE;
					e.type = EVENT_MOUSE_ENTER;
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

				case KeyPress:
					if (has_focus) {
						e.scancode = fb_x11keycode_to_scancode[event.xkey.keycode];
						e.ascii = 0;
						__fb_gfx->key[e.scancode] = TRUE;

						if( __fb_gfx->key[SC_ENTER] && __fb_gfx->key[SC_ALT] && !(fb_x11.flags & DRIVER_NO_SWITCH) ) {
							hOnAltEnter( );
						} else {
							key = translate_key( &event, e.scancode );
							if( key ) {
								fb_hPostKey( key );
								/* Don't return extended keycodes in the ascii field */
								e.ascii = ((key < 0) || (key > 0xFF)) ? 0 : key;
							}
						}

						e.type = EVENT_KEY_PRESS;
					}
					break;

				case KeyRelease:
					if (has_focus) {
						e.scancode = fb_x11keycode_to_scancode[event.xkey.keycode];
						key = translate_key( &event, e.scancode );
						/* Don't return extended keycodes in the ascii field */
						e.ascii = ((key < 0) || (key > 0xFF)) ? 0 : key;
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
			}
			if (e.type)
				fb_hPostEvent(&e);
		}

		pthread_cond_signal(&cond);

		fb_hX11Unlock();

		usleep(30000);
	}

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
	if (XGrabKeyboard(fb_x11.display, root_window, False,
	    GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess)
		return -1;

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
		XUngrabKeyboard(fb_x11.display, CurrentTime);
		current_size = orig_size;
		__fb_gfx->refresh_rate = fb_x11.refresh_rate = orig_rate;
	}
}

void WaitMapped(Window w)
{
	XEvent e;
	do {
		XMaskEvent(fb_x11.display, StructureNotifyMask, &e);
	} while ((e.type != MapNotify) || (e.xmap.event != w));
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
	fb_hX11SetWindowTitle( title );
#ifndef DISABLE_XPM
	if (fb_program_icon) {
		hints.flags = IconPixmapHint | IconMaskHint;
		xpm_attribs.valuemask = XpmReturnAllocPixels | XpmReturnExtensions;
		XpmCreatePixmapFromData(fb_x11.display, fb_x11.window, fb_program_icon, &hints.icon_pixmap, &hints.icon_mask, &xpm_attribs);
		XSetWMHints(fb_x11.display, fb_x11.wmwindow, &hints);
	}
#endif

	size = XAllocSizeHints();
	size->flags = PBaseSize | PMinSize | PMaxSize | PResizeInc;
	size->base_width = fb_x11.content_w;
	size->base_height = fb_x11.content_h;
	if (flags & DRIVER_RESIZABLE) {
		size->min_width = 8;
		size->min_height = (__fb_gfx && __fb_gfx->font) ?
			__fb_gfx->font->h * __fb_gfx->scanline_size : 16;
		size->max_width = XDisplayWidth(fb_x11.display, fb_x11.screen);
		size->max_height = XDisplayHeight(fb_x11.display, fb_x11.screen);
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
		mouse_x = fb_x11.content_w >> 1;
		mouse_y = fb_x11.content_h >> 1;
	} else {
		has_focus = FALSE;
		mouse_on = FALSE;
	}
	mouse_buttons = mouse_latched_buttons = mouse_wheel = 0;

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	pthread_mutex_lock(&mutex);
	if (!pthread_create(&thread, NULL, window_thread, NULL)) {
		pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);
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
	if (is_running) {
		is_running = FALSE;
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

int fb_hX11GetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	Window root, child;
	int root_x, root_y, win_x, win_y;
	unsigned int buttons_mask;
	const int invalid_coord = -1;

	if (x)
		*x = invalid_coord;
	if (y)
		*y = invalid_coord;
	if (buttons)
		*buttons = 0;

	/* prefer XQueryPointer to have a more responsive mouse position retrieval */
	if (z) *z = mouse_wheel;
	/* Keep a short-press visible once even if polling sees pointer outside. */
	if ((!mouse_on) && (!mouse_latched_buttons) && (buttons == NULL))
		return 0;

	if (XQueryPointer(fb_x11.display, fb_x11.window, &root, &child, &root_x, &root_y, &win_x, &win_y, &buttons_mask)) {
		if (x) *x = win_x;
		if (y) *y = win_y;
		hWindowToFramebuffer(win_x, win_y, x, y, NULL);
		if (buttons) {
			*buttons = (buttons_mask & Button1Mask ? 0x1 : 0) |
				   (buttons_mask & Button3Mask ? 0x2 : 0) |
				   (buttons_mask & Button2Mask ? 0x4 : 0);
			*buttons |= mouse_latched_buttons;
		}
	} else {
		if (x) *x = mouse_x;
		if (y) *y = mouse_y;
		if (buttons) *buttons = mouse_buttons | mouse_latched_buttons;
	}
	if (buttons) {
		mouse_latched_buttons = 0;
	}

	if (clip) *clip = fb_x11.mouse_clip;
	return 0;
}

void fb_hX11SetMouse(int x, int y, int show, int clip)
{
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

		hFramebufferToWindow(mouse_x, mouse_y, &x, &y);
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
		fb_x11.mouse_clip = FALSE;
		XUngrabPointer(fb_x11.display, CurrentTime);
	}
	else if (clip > 0) {
		fb_x11.mouse_clip = TRUE;
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
