/*
    FreeBASIC gfxlib2 Wii backend
    -----------------------------

    File: gfx_driver.c

    Purpose:

        Present the gfxlib framebuffer through libogc's external framebuffer.

    Responsibilities:

        - open a Wii VI framebuffer mode
        - convert gfxlib's framebuffer into the Wii YUYV XFB format
        - scale lower-resolution gfxlib modes into the TV framebuffer
        - present from explicit update calls and input polling boundaries
        - expose Wiimote IR pointing as the gfxlib mouse
        - expose a small Wii/GC controller-to-keyboard compatibility bridge

    This file intentionally does NOT contain:

        - general controller mapping, which lives in gfx_xpad.c
        - sound playback
        - filesystem or application lifecycle logic

    Platform notes:

        libogc's external framebuffer is not RGB.  It is a packed YUYV buffer
        using one chroma pair for every two horizontal pixels.  Most modes use
        gfxlib's normal blitter to build a 32-bit RGB staging buffer before
        conversion.  The 16-bit path samples the gfxlib 565 pixels directly so
        it does not depend on word packing in the shared blitter on big-endian
        PowerPC.  Old visible-page BASIC programs often draw a complete frame
        and then wait for input without calling SCREENSET or SCREENCOPY.  The
        Wii backend treats input polling as a safe boundary for those programs:
        it presents the completed dirty frame there, but still avoids
        presenting from every primitive unlock.
*/

#include "../fb_gfx.h"

#include <gccore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>

#define SCREENLIST(w, h) ((h) | (w) << 16)
#define WII_XFB_COUNT 2
#define WII_MOUSE_KEEP ((int)0x80000000)

static BLITTER *blitter;
static uint32_t *rgb_buffer;
static size_t rgb_buffer_size;
static GXRModeObj *rmode;
static void *xfb[WII_XFB_COUNT];
static int visible_xfb;
static int video_w;
static int video_h;
static volatile int quitting;
static int mouse_x;
static int mouse_y;
static int mouse_z;
static int mouse_buttons;
static int mouse_latched_buttons;
static int mouse_clip;
static int mouse_cursor;
static int keyboard_escape_down;
static u32 keyboard_buttons;
static u32 keyboard_dpad;

static int ensure_rgb_buffer(size_t pixels)
{
	uint32_t *new_buffer;

	if (rgb_buffer_size >= pixels)
		return TRUE;

	if (pixels > (SIZE_MAX / sizeof(uint32_t)))
		return FALSE;

	new_buffer = (uint32_t *)realloc(rgb_buffer, pixels * sizeof(uint32_t));
	if (!new_buffer) {
		free(rgb_buffer);
		rgb_buffer = NULL;
		rgb_buffer_size = 0;
		return FALSE;
	}

	rgb_buffer = new_buffer;
	rgb_buffer_size = pixels;
	return TRUE;
}

static int has_dirty_lines(void)
{
	int y;

	if (!__fb_gfx || !__fb_gfx->dirty)
		return FALSE;

	for (y = 0; y < __fb_gfx->h; ++y) {
		if (__fb_gfx->dirty[y])
			return TRUE;
	}

	return FALSE;
}

static int wii_clamp_int(int value, int low, int high)
{
	if (value < low)
		return low;
	if (value > high)
		return high;
	return value;
}

static void wii_get_mouse_bounds(int *width, int *height)
{
	int w = video_w;
	int h = video_h;

	if (__fb_gfx) {
		w = __fb_gfx->w;
		h = __fb_gfx->h;
	}

	if (w <= 0)
		w = 1;
	if (h <= 0)
		h = 1;

	*width = w;
	*height = h;
}

static int wii_mouse_buttons_from_wpad(u32 buttons)
{
	int fb_buttons = 0;

	/*
		Wiimote pointing is exposed as the regular gfxlib mouse so existing
		BASIC code can use GETMOUSE without knowing that the "mouse" is an IR
		camera.  The mapping follows the common Wii menu convention: A selects,
		B is the secondary action, and 1 is the extra/middle button.
	*/
	if (buttons & WPAD_BUTTON_A)
		fb_buttons |= BUTTON_LEFT;
	if (buttons & WPAD_BUTTON_B)
		fb_buttons |= BUTTON_RIGHT;
	if (buttons & WPAD_BUTTON_1)
		fb_buttons |= BUTTON_MIDDLE;

	return fb_buttons;
}

static int wii_wpad_is_connected(int channel)
{
	u32 type;

	if ((channel < WPAD_CHAN_0) || (channel > WPAD_CHAN_3))
		return FALSE;

	return WPAD_Probe(channel, &type) == WPAD_ERR_NONE;
}

static void wii_move_mouse_with_dpad(u32 dpad)
{
	int width;
	int height;
	int dx = 0;
	int dy = 0;

	if (dpad & WPAD_BUTTON_LEFT)
		dx -= 2;
	if (dpad & WPAD_BUTTON_RIGHT)
		dx += 2;
	if (dpad & WPAD_BUTTON_UP)
		dy -= 2;
	if (dpad & WPAD_BUTTON_DOWN)
		dy += 2;

	if ((dx == 0) && (dy == 0))
		return;

	wii_get_mouse_bounds(&width, &height);
	mouse_x = wii_clamp_int(mouse_x + dx, 0, width - 1);
	mouse_y = wii_clamp_int(mouse_y + dy, 0, height - 1);
}

static int wii_find_pointer_channel(ir_t *ir, int *channel)
{
	int i;
	int first_connected = -1;
	u32 type;
	ir_t current_ir;

	for (i = WPAD_CHAN_0; i <= WPAD_CHAN_3; ++i) {
		if (WPAD_Probe(i, &type) != WPAD_ERR_NONE)
			continue;

		if (first_connected < 0)
			first_connected = i;

		current_ir.valid = FALSE;
		WPAD_IR(i, &current_ir);
		if (current_ir.valid) {
			*ir = current_ir;
			*channel = i;
			return TRUE;
		}
	}

	if (first_connected >= 0) {
		ir->valid = FALSE;
		*channel = first_connected;
		return TRUE;
	}

	return FALSE;
}

static void wii_set_key_state(int scancode, int down)
{
	if (__fb_gfx && __fb_gfx->key && (scancode >= 0) && (scancode < 128))
		__fb_gfx->key[scancode] = down ? TRUE : FALSE;
}

static void wii_post_key_on_press(int down, int was_down, int key)
{
	if (down && !was_down)
		fb_hPostKey(key);
}

static void wii_update_keyboard(void)
{
	int i;
	u32 buttons = 0;
	u32 dpad = 0;

	PAD_ScanPads();
	WPAD_ScanPads();

	for (i = PAD_CHAN0; i < PAD_CHANMAX; ++i) {
		u16 held = PAD_ButtonsHeld(i);

		if (held & PAD_BUTTON_A)
			buttons |= WPAD_BUTTON_A;
		if (held & PAD_BUTTON_B)
			buttons |= WPAD_BUTTON_B;
		if (held & PAD_BUTTON_START)
			buttons |= WPAD_BUTTON_PLUS;
		if (held & PAD_BUTTON_X)
			buttons |= WPAD_BUTTON_1;
		if (held & PAD_BUTTON_Y)
			buttons |= WPAD_BUTTON_2;
		if (held & PAD_TRIGGER_Z)
			buttons |= WPAD_BUTTON_MINUS;

		if (held & PAD_BUTTON_UP)
			dpad |= WPAD_BUTTON_UP;
		if (held & PAD_BUTTON_RIGHT)
			dpad |= WPAD_BUTTON_RIGHT;
		if (held & PAD_BUTTON_DOWN)
			dpad |= WPAD_BUTTON_DOWN;
		if (held & PAD_BUTTON_LEFT)
			dpad |= WPAD_BUTTON_LEFT;
	}

	for (i = WPAD_CHAN_0; i <= WPAD_CHAN_3; ++i) {
		u32 type;
		u32 held;

		if (WPAD_Probe(i, &type) != WPAD_ERR_NONE)
			continue;

		held = WPAD_ButtonsHeld(i);
		buttons |= held;

		if (held & WPAD_BUTTON_UP)
			dpad |= WPAD_BUTTON_UP;
		if (held & WPAD_BUTTON_RIGHT)
			dpad |= WPAD_BUTTON_RIGHT;
		if (held & WPAD_BUTTON_DOWN)
			dpad |= WPAD_BUTTON_DOWN;
		if (held & WPAD_BUTTON_LEFT)
			dpad |= WPAD_BUTTON_LEFT;

#ifdef WPAD_CLASSIC_BUTTON_UP
		if (held & WPAD_CLASSIC_BUTTON_UP)
			dpad |= WPAD_BUTTON_UP;
		if (held & WPAD_CLASSIC_BUTTON_RIGHT)
			dpad |= WPAD_BUTTON_RIGHT;
		if (held & WPAD_CLASSIC_BUTTON_DOWN)
			dpad |= WPAD_BUTTON_DOWN;
		if (held & WPAD_CLASSIC_BUTTON_LEFT)
			dpad |= WPAD_BUTTON_LEFT;
#endif

#ifdef WPAD_CLASSIC_BUTTON_A
		if (held & WPAD_CLASSIC_BUTTON_A)
			buttons |= WPAD_BUTTON_A;
		if (held & WPAD_CLASSIC_BUTTON_B)
			buttons |= WPAD_BUTTON_B;
#endif
	}

	/*
		The Wii has no PC keyboard by default, but many old FreeBASIC games use
		INKEY$, GETKEY, SLEEP, or MULTIKEY for title screens and first-player
		movement.  Keep the mapping small and conventional: A is Space, +/Start
		is a start/confirm button that also queues "1" for numbered menus,
		B/-/Home are Escape, and the d-pad is the cursor keys.  Programs that
		need exact controller state can still use GETXPAD.
	*/
	DRIVER_LOCK();

	wii_post_key_on_press(buttons & WPAD_BUTTON_A,
	                      keyboard_buttons & WPAD_BUTTON_A,
	                      ' ');
	wii_post_key_on_press(buttons & WPAD_BUTTON_PLUS,
	                      keyboard_buttons & WPAD_BUTTON_PLUS,
	                      '1');
	wii_post_key_on_press(buttons & WPAD_BUTTON_PLUS,
	                      keyboard_buttons & WPAD_BUTTON_PLUS,
	                      '\r');
	wii_post_key_on_press(buttons & WPAD_BUTTON_1,
	                      keyboard_buttons & WPAD_BUTTON_1,
	                      '1');
	wii_post_key_on_press(buttons & WPAD_BUTTON_2,
	                      keyboard_buttons & WPAD_BUTTON_2,
	                      '2');
	wii_post_key_on_press(buttons & (WPAD_BUTTON_B | WPAD_BUTTON_MINUS |
	                      WPAD_BUTTON_HOME),
	                      keyboard_buttons & (WPAD_BUTTON_B |
	                      WPAD_BUTTON_MINUS | WPAD_BUTTON_HOME),
	                      27);
	wii_post_key_on_press(dpad & WPAD_BUTTON_LEFT,
	                      keyboard_dpad & WPAD_BUTTON_LEFT,
	                      fb_hScancodeToExtendedKey(SC_LEFT));
	wii_post_key_on_press(dpad & WPAD_BUTTON_RIGHT,
	                      keyboard_dpad & WPAD_BUTTON_RIGHT,
	                      fb_hScancodeToExtendedKey(SC_RIGHT));
	wii_post_key_on_press(dpad & WPAD_BUTTON_UP,
	                      keyboard_dpad & WPAD_BUTTON_UP,
	                      fb_hScancodeToExtendedKey(SC_UP));
	wii_post_key_on_press(dpad & WPAD_BUTTON_DOWN,
	                      keyboard_dpad & WPAD_BUTTON_DOWN,
	                      fb_hScancodeToExtendedKey(SC_DOWN));

	wii_set_key_state(SC_1, buttons & (WPAD_BUTTON_1 | WPAD_BUTTON_PLUS));
	wii_set_key_state(SC_2, buttons & WPAD_BUTTON_2);
	wii_set_key_state(SC_SPACE, buttons & (WPAD_BUTTON_A | WPAD_BUTTON_PLUS));
	wii_set_key_state(SC_ENTER, buttons & WPAD_BUTTON_PLUS);
	wii_set_key_state(SC_ESCAPE, buttons & (WPAD_BUTTON_B |
	                  WPAD_BUTTON_MINUS | WPAD_BUTTON_HOME));
	wii_set_key_state(SC_LEFT, dpad & WPAD_BUTTON_LEFT);
	wii_set_key_state(SC_RIGHT, dpad & WPAD_BUTTON_RIGHT);
	wii_set_key_state(SC_UP, dpad & WPAD_BUTTON_UP);
	wii_set_key_state(SC_DOWN, dpad & WPAD_BUTTON_DOWN);

	keyboard_escape_down = (buttons & (WPAD_BUTTON_B | WPAD_BUTTON_MINUS |
	                       WPAD_BUTTON_HOME)) ? TRUE : FALSE;
	keyboard_buttons = buttons;
	keyboard_dpad = dpad;

	DRIVER_UNLOCK();
}

static void wii_update_mouse(void)
{
	int channel;
	int width;
	int height;
	ir_t ir;
	u32 held;
	u32 down;

	wii_update_keyboard();

	if (!wii_find_pointer_channel(&ir, &channel)) {
		wii_move_mouse_with_dpad(keyboard_dpad);
		mouse_buttons = wii_mouse_buttons_from_wpad(keyboard_buttons);
		if (mouse_buttons == 0)
			mouse_latched_buttons = 0;
		return;
	}

	if (ir.valid && !keyboard_dpad) {
		wii_get_mouse_bounds(&width, &height);
		mouse_x = wii_clamp_int((int)(ir.x + 0.5f), 0, width - 1);
		mouse_y = wii_clamp_int((int)(ir.y + 0.5f), 0, height - 1);
	} else {
		wii_move_mouse_with_dpad(keyboard_dpad);
	}

	if (!wii_wpad_is_connected(channel)) {
		mouse_buttons = 0;
		mouse_latched_buttons = 0;
		return;
	}

	held = WPAD_ButtonsHeld(channel);
	down = WPAD_ButtonsDown(channel);

	if (down & WPAD_BUTTON_PLUS)
		++mouse_z;
	if (down & WPAD_BUTTON_MINUS)
		--mouse_z;

	mouse_latched_buttons |= wii_mouse_buttons_from_wpad(down);
	mouse_buttons = wii_mouse_buttons_from_wpad(held) | mouse_latched_buttons;
}

static unsigned char clamp_u8(int value)
{
	if (value < 0)
		return 0;
	if (value > 255)
		return 255;
	return (unsigned char)value;
}

static void rgb_to_yuv(uint32_t color, int *y, int *u, int *v)
{
	int r = (int)((color >> 16) & 0xff);
	int g = (int)((color >> 8) & 0xff);
	int b = (int)(color & 0xff);

	*y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
	*u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
	*v = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
}

static void write_yuyv_pair(unsigned char *dst, uint32_t left, uint32_t right)
{
	int y0, u0, v0;
	int y1, u1, v1;
	uint32_t packed;

	rgb_to_yuv(left, &y0, &u0, &v0);
	rgb_to_yuv(right, &y1, &u1, &v1);

	/*
		The external framebuffer is normally handled through 32-bit YUYV
		values, as shown by libogc's COLOR_* constants.  On Wii hardware and in
		Dolphin, byte stores into the XFB can leave the earlier bytes of the
		word unchanged.  Pack and store the whole pair at once so all channels
		reach video memory together.
	*/
	packed = ((uint32_t)clamp_u8(y0) << 24) |
		((uint32_t)clamp_u8((u0 + u1) / 2) << 16) |
		((uint32_t)clamp_u8(y1) << 8) |
		(uint32_t)clamp_u8((v0 + v1) / 2);

	*((uint32_t *)dst) = packed;
}

static uint32_t sample_16bpp_pixel(int x, int y)
{
	unsigned char *row;
	uint16_t pixel;
	uint32_t r;
	uint32_t g;
	uint32_t b;

	row = __fb_gfx->framebuffer + (y * __fb_gfx->pitch);
	pixel = ((uint16_t *)row)[x];

	r = (uint32_t)((pixel & MASK_R_16) >> 11);
	g = (uint32_t)((pixel & MASK_G_16) >> 5);
	b = (uint32_t)(pixel & MASK_B_16);

	r = (r << 3) | (r >> 2);
	g = (g << 2) | (g >> 4);
	b = (b << 3) | (b >> 2);

	return (r << 16) | (g << 8) | b;
}

static uint32_t sample_source_pixel(int dst_x, int dst_y)
{
	int src_x;
	int src_y;

	src_x = (dst_x * __fb_gfx->w) / video_w;
	src_y = (dst_y * __fb_gfx->h) / video_h;

	if (src_x < 0)
		src_x = 0;
	else if (src_x >= __fb_gfx->w)
		src_x = __fb_gfx->w - 1;

	if (src_y < 0)
		src_y = 0;
	else if (src_y >= __fb_gfx->h)
		src_y = __fb_gfx->h - 1;

	if (__fb_gfx->depth == 16)
		return sample_16bpp_pixel(src_x, src_y);

	return rgb_buffer[(src_y * __fb_gfx->w) + src_x];
}

static void clear_xfb(void *buffer)
{
	uint32_t *dst = (uint32_t *)buffer;
	u32 size;
	u32 i;

	size = VIDEO_GetFrameBufferSize(rmode) / sizeof(uint32_t);

	for (i = 0; i < size; ++i)
		dst[i] = 0x10801080;
}

/* ------------------------------------------------------------------------- */
/* Optional presented-frame diagnostics                                      */
/* ------------------------------------------------------------------------- */

/*
	FBGFX_WII_XFB_DUMP_PREFIX is a smoke-test aid for the Wii backend.

	The normal gfxlib framebuffer is RGB, but the Wii external framebuffer
	receives packed YUYV.  Dumping after conversion lets automated tests check
	the same pixels Dolphin or hardware will present, including the chroma
	sharing that happens between each pair of pixels.
*/

static int dump_initialized;
static char dump_prefix_storage[192];
static const char *dump_prefix;
static int dump_limit;
static int dump_count;

static void init_xfb_dump(void)
{
	const char *prefix_text;
	const char *limit_text;
	int new_limit;

	prefix_text = getenv("FBGFX_WII_XFB_DUMP_PREFIX");
	if (!prefix_text || !*prefix_text) {
		dump_initialized = TRUE;
		dump_prefix_storage[0] = '\0';
		dump_prefix = NULL;
		dump_limit = 0;
		dump_count = 0;
		return;
	}

	limit_text = getenv("FBGFX_WII_XFB_DUMP_FRAMES");
	new_limit = 0;

	if (limit_text && *limit_text)
		new_limit = atoi(limit_text);

	if (new_limit <= 0)
		new_limit = 32;

	/*
		Smoke tests change the prefix between screen modes so each presented
		frame can be tied back to the mode that produced it.  The ordinary path
		only pays for this getenv() check when diagnostics are enabled.
	*/
	if (!dump_initialized ||
		strncmp(dump_prefix_storage, prefix_text,
			sizeof(dump_prefix_storage)) != 0) {
		strncpy(dump_prefix_storage, prefix_text,
			sizeof(dump_prefix_storage) - 1);
		dump_prefix_storage[sizeof(dump_prefix_storage) - 1] = '\0';
		dump_count = 0;
	}

	dump_initialized = TRUE;
	dump_prefix = dump_prefix_storage;
	dump_limit = new_limit;
}

static void yuv_to_rgb(unsigned char y, unsigned char u, unsigned char v,
	unsigned char *r, unsigned char *g, unsigned char *b)
{
	int c = (int)y - 16;
	int d = (int)u - 128;
	int e = (int)v - 128;

	if (c < 0)
		c = 0;

	*r = clamp_u8((298 * c + 409 * e + 128) >> 8);
	*g = clamp_u8((298 * c - 100 * d - 208 * e + 128) >> 8);
	*b = clamp_u8((298 * c + 516 * d + 128) >> 8);
}

static void dump_xfb_samples(void *buffer)
{
	unsigned char *xfb_bytes = (unsigned char *)buffer;
	char filename[256];
	FILE *file;
	int y;
	int i;
	int xs[3];

	if (snprintf(filename, sizeof(filename), "%s-%04d.txt",
		dump_prefix, dump_count) >= (int)sizeof(filename)) {
		return;
	}

	file = fopen(filename, "wb");
	if (!file)
		return;

	y = video_h / 2;
	xs[0] = video_w / 6;
	xs[1] = video_w / 2;
	xs[2] = (video_w * 5) / 6;

	fprintf(file, "depth=%d source=%dx%d video=%dx%d\n",
		__fb_gfx ? __fb_gfx->depth : 0,
		__fb_gfx ? __fb_gfx->w : 0,
		__fb_gfx ? __fb_gfx->h : 0,
		video_w,
		video_h);

	for (i = 0; i < 3; ++i) {
		int x = xs[i];
		int pair = x & ~1;
		unsigned char *pixel = xfb_bytes + (y * video_w * 2) + (pair * 2);
		uint32_t source = sample_source_pixel(x, y);

		fprintf(file, "x=%d source=%08x yuyv=%02x %02x %02x %02x\n",
			x,
			(unsigned int)source,
			pixel[0],
			pixel[1],
			pixel[2],
			pixel[3]);
	}

	fclose(file);
}

static void dump_xfb_as_ppm(void *buffer)
{
	unsigned char *src = (unsigned char *)buffer;
	unsigned char rgb[3];
	char filename[256];
	FILE *file;
	int x;
	int y;

	init_xfb_dump();

	if (!dump_prefix || !*dump_prefix)
		return;

	if (dump_count >= dump_limit)
		return;

	if (snprintf(filename, sizeof(filename), "%s-%04d.ppm",
		dump_prefix, dump_count) >= (int)sizeof(filename)) {
		return;
	}

	file = fopen(filename, "wb");
	if (!file)
		return;

	dump_xfb_samples(buffer);

	fprintf(file, "P6\n%d %d\n255\n", video_w, video_h);

	for (y = 0; y < video_h; ++y) {
		unsigned char *row = src + (y * video_w * 2);

		for (x = 0; x < video_w; x += 2) {
			unsigned char y0 = row[(x * 2) + 0];
			unsigned char u = row[(x * 2) + 1];
			unsigned char y1 = row[(x * 2) + 2];
			unsigned char v = row[(x * 2) + 3];

			yuv_to_rgb(y0, u, v, &rgb[0], &rgb[1], &rgb[2]);
			fwrite(rgb, 1, sizeof(rgb), file);

			if (x + 1 < video_w) {
				yuv_to_rgb(y1, u, v, &rgb[0], &rgb[1], &rgb[2]);
				fwrite(rgb, 1, sizeof(rgb), file);
			}
		}
	}

	fclose(file);
	++dump_count;
}

static void driver_present_framebuffer(void)
{
	unsigned char *dst;
	int back;
	int x;
	int y;

	if (!__fb_gfx || !blitter || !rmode || !xfb[0] || !xfb[1])
		return;

	if (!has_dirty_lines())
		return;

	if (__fb_gfx->depth != 16) {
		if (!ensure_rgb_buffer((size_t)__fb_gfx->w * (size_t)__fb_gfx->h))
			return;

		blitter((unsigned char *)rgb_buffer,
			__fb_gfx->w * (int)sizeof(uint32_t));
	}

	back = visible_xfb ^ 1;
	dst = (unsigned char *)xfb[back];

	for (y = 0; y < video_h; ++y) {
		unsigned char *row = dst + (y * video_w * 2);

		for (x = 0; x < video_w; x += 2) {
			uint32_t left = sample_source_pixel(x, y);
			uint32_t right = sample_source_pixel((x + 1 < video_w) ? x + 1 : x, y);

			write_yuyv_pair(row + (x * 2), left, right);
		}
	}

	dump_xfb_as_ppm(xfb[back]);
	DCFlushRange(xfb[back], VIDEO_GetFrameBufferSize(rmode));
	VIDEO_SetNextFramebuffer(xfb[back]);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	visible_xfb = back;
	fb_hMemSet(__fb_gfx->dirty, FALSE, __fb_gfx->h);
}

static int driver_init(char *title, int w, int h, int depth_arg, int refresh_rate, int flags)
{
	int i;

	(void)title;
	(void)w;
	(void)h;
	(void)depth_arg;
	(void)refresh_rate;

	if (flags & DRIVER_OPENGL)
		return -1;

	fb_WiiVideoInit();
	rmode = fb_WiiGetRenderMode();
	if (rmode == NULL)
		return -1;

	video_w = rmode->fbWidth;
	video_h = rmode->xfbHeight;
	mouse_x = (w > 0) ? (w / 2) : (video_w / 2);
	mouse_y = (h > 0) ? (h / 2) : (video_h / 2);
	mouse_z = 0;
	mouse_buttons = 0;
	mouse_latched_buttons = 0;
	mouse_clip = FALSE;
	mouse_cursor = TRUE;
	keyboard_escape_down = FALSE;
	keyboard_buttons = 0;
	keyboard_dpad = 0;

	/*
		Enable the Wiimote reports needed by both pointer mouse support and
		GETXPAD motion support.  libogc keeps the last report for each channel,
		so the gfx and xpad paths can read it without maintaining a separate
		event thread.
	*/
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL,
		(w > 0) ? (u32)w : (u32)video_w,
		(h > 0) ? (u32)h : (u32)video_h);

	for (i = 0; i < WII_XFB_COUNT; ++i) {
		if (!xfb[i])
			xfb[i] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
		if (!xfb[i])
			return -1;
		clear_xfb(xfb[i]);
		DCFlushRange(xfb[i], VIDEO_GetFrameBufferSize(rmode));
	}

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb[0]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	visible_xfb = 0;
	blitter = fb_hGetBlitter(32, FALSE);
	quitting = FALSE;

	return blitter ? 0 : -1;
}

static void driver_exit(void)
{
	quitting = TRUE;
	blitter = NULL;
	video_w = 0;
	video_h = 0;
	free(rgb_buffer);
	rgb_buffer = NULL;
	rgb_buffer_size = 0;
}

static void driver_lock(void)
{
}

static void driver_unlock(void)
{
	/*
		One-page QB-style graphics modes draw directly to the visible page.
		Those programs often build a screen and then wait for input without ever
		calling SCREENSET or SCREENCOPY, so the backend must expose visible-page
		changes as drawing completes.

		Multi-page programs are different: their frame boundary is expressed
		through SCREENSET/SCREENCOPY and the gfxlib update hook.  Presenting from
		unlock there would turn a single logical frame into many hardware
		updates and expose partially drawn offscreen pages.
	*/
	if (!quitting && __fb_gfx && (__fb_gfx->num_pages <= 1))
		driver_present_framebuffer();
}

static void driver_update(void)
{
	if (!quitting)
		driver_present_framebuffer();
}

static void driver_set_palette(int index, int r, int g, int b)
{
	(void)index;
	(void)r;
	(void)g;
	(void)b;
}

static void driver_wait_vsync(void)
{
	/*
		SCREENLOCK/SCREENUNLOCK protects drawing, but it is too fine-grained as
		a presentation boundary because many primitives lock internally.  On
		Wii, SCREENSYNC is an explicit point where a visible-page program is
		asking to synchronize with display hardware, so flush any dirty visible
		frame before waiting for the next retrace.
	*/
	driver_present_framebuffer();
	VIDEO_WaitVSync();
}

static int driver_get_mouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	wii_update_mouse();
	driver_present_framebuffer();

	if (x) *x = mouse_x;
	if (y) *y = mouse_y;
	if (z) *z = mouse_z;
	if (buttons) *buttons = mouse_buttons;
	if (clip) *clip = mouse_clip;

	mouse_latched_buttons = 0;
	return 0;
}

static void driver_set_mouse(int x, int y, int cursor, int clip)
{
	int width;
	int height;

	wii_get_mouse_bounds(&width, &height);

	if ((x != WII_MOUSE_KEEP) || (y != WII_MOUSE_KEEP)) {
		if (x == WII_MOUSE_KEEP)
			x = mouse_x;
		if (y == WII_MOUSE_KEEP)
			y = mouse_y;

		mouse_x = wii_clamp_int(x, 0, width - 1);
		mouse_y = wii_clamp_int(y, 0, height - 1);
	}

	if (cursor >= 0)
		mouse_cursor = (cursor != 0);
	if (clip == 0)
		mouse_clip = FALSE;
	else if (clip > 0)
		mouse_clip = TRUE;
}

static int *driver_fetch_modes(int depth, int *size)
{
	int *modes;

	(void)depth;

	modes = (int *)malloc(sizeof(int) * 3);
	if (!modes) {
		*size = 0;
		return NULL;
	}

	modes[0] = SCREENLIST(320, 240);
	modes[1] = SCREENLIST(640, 480);
	modes[2] = SCREENLIST(640, 528);
	*size = 3;
	return modes;
}

static void driver_poll_events(void)
{
	/*
		A visible-page BASIC program can draw a complete frame and then wait for
		input without calling SCREENSET or SCREENCOPY.  Poll-events is a safe
		place to expose that completed frame, and it is also where INKEY$,
		GETKEY, SLEEP, and MULTIKEY see the controller-to-keyboard bridge.
	*/
	wii_update_keyboard();
	driver_present_framebuffer();
}

static const GFXDRIVER fb_gfxDriverWii =
{
	"wii",
	driver_init,
	driver_exit,
	driver_lock,
	driver_unlock,
	driver_set_palette,
	driver_wait_vsync,
	driver_get_mouse,
	NULL,
	NULL,
	driver_set_mouse,
	NULL,
	NULL,
	driver_fetch_modes,
	NULL,
	driver_poll_events,
	driver_update
};

const GFXDRIVER *__fb_gfx_drivers_list[] = {
	&fb_gfxDriverWii,
	NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	GXRModeObj *info = fb_WiiGetRenderMode();

	*width = info ? info->fbWidth : 640;
	*height = info ? info->xfbHeight : 480;
	*depth = 32;
	*refresh = 60;
}

ssize_t fb_hGetWindowHandle(void)
{
	/*
		SCREENCONTROL can expose a host window/display handle on desktop
		targets.  Wii homebrew renders directly through GX/XFB state, so there
		is no native handle that can be handed to user code.
	*/
	return 0;
}

ssize_t fb_hGetDisplayHandle(void)
{
	return 0;
}

void *fb_hGL_GetProcAddress(const char *proc)
{
	(void)proc;
	return NULL;
}

FBCALL int fb_GfxGetJoystick(int id, ssize_t *buttons, float *a1, float *a2, float *a3, float *a4, float *a5, float *a6, float *a7, float *a8)
{
	ssize_t xpad_buttons;
	ssize_t dpad;
	float lx, ly, rx, ry, lt, rt;
	int status;

	if (buttons) *buttons = -1;
	if (a1) *a1 = -1000.0f;
	if (a2) *a2 = -1000.0f;
	if (a3) *a3 = -1000.0f;
	if (a4) *a4 = -1000.0f;
	if (a5) *a5 = -1000.0f;
	if (a6) *a6 = -1000.0f;
	if (a7) *a7 = -1000.0f;
	if (a8) *a8 = -1000.0f;

	if ((id < 0) || (id >= 4))
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

	status = fb_GfxGetXPad(id, &xpad_buttons, &lx, &ly, &rx, &ry, &lt, &rt, &dpad);
	if (status != XPAD_STATUS_CONNECTED)
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);

	if (buttons) *buttons = xpad_buttons;
	if (a1) *a1 = lx;
	if (a2) *a2 = ly;
	if (a3) *a3 = lt;
	if (a4) *a4 = rx;
	if (a5) *a5 = ry;
	if (a6) *a6 = rt;
	if (a7) *a7 = (dpad & XPAD_DPAD_RIGHT) ? 1.0f : ((dpad & XPAD_DPAD_LEFT) ? -1.0f : 0.0f);
	if (a8) *a8 = (dpad & XPAD_DPAD_DOWN) ? 1.0f : ((dpad & XPAD_DPAD_UP) ? -1.0f : 0.0f);

	return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* end of gfx_driver.c */
