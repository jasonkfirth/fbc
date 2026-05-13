/*
    FreeBASIC gfxlib2 Xbox backend
    ------------------------------

    File: gfx_driver.c

    Purpose:

        Present the gfxlib framebuffer through nxdk's XVideo framebuffer.

    Responsibilities:

        - open a real Xbox video mode
        - convert gfxlib's framebuffer to 32-bit pixels
        - present lower-resolution gfxlib modes with integer scaling
        - fall back to integer pixel skipping when the requested buffer is
          larger than the selected video mode

    This file intentionally does NOT contain:

        - input handling
        - window management
        - audio or lifecycle handling
*/

#include "../fb_gfx.h"
#include <hal/xbox.h>
#include <hal/video.h>
#include <stdint.h>

#define SCREENLIST(w, h) ((h) | (w) << 16)
#define XBOX_DEFAULT_W 640
#define XBOX_DEFAULT_H 480

static BLITTER *blitter;
static void *framebuffer;
static uint32_t *scale_buffer;
static size_t scale_buffer_size;
static int video_w;
static int video_h;
static volatile int quitting;

static int ensure_scale_buffer(size_t pixels)
{
	uint32_t *new_buffer;

	if (scale_buffer_size >= pixels)
		return TRUE;

	if (pixels > (SIZE_MAX / sizeof(uint32_t)))
		return FALSE;

	new_buffer = (uint32_t *)realloc(scale_buffer, pixels * sizeof(uint32_t));
	if (!new_buffer) {
		free(scale_buffer);
		scale_buffer = NULL;
		scale_buffer_size = 0;
		return FALSE;
	}

	scale_buffer = new_buffer;
	scale_buffer_size = pixels;
	return TRUE;
}

static void choose_video_mode(int src_w, int src_h, int *out_w, int *out_h)
{
	VIDEO_MODE vm;
	void *p = NULL;
	int best_area = 0;

	*out_w = XBOX_DEFAULT_W;
	*out_h = XBOX_DEFAULT_H;

	while (XVideoListModes(&vm, 32, 0, &p)) {
		int area;

		if ((vm.width < src_w) || (vm.height < src_h))
			continue;

		area = vm.width * vm.height;
		if ((best_area == 0) || (area < best_area)) {
			*out_w = vm.width;
			*out_h = vm.height;
			best_area = area;
		}
	}
}

static void get_viewport(int *scale, int *skip, int *out_w, int *out_h, int *off_x, int *off_y)
{
	int src_w = __fb_gfx->w;
	int src_h = __fb_gfx->h;
	int local_scale;
	int local_skip;

	local_scale = video_w / src_w;
	if ((video_h / src_h) < local_scale)
		local_scale = video_h / src_h;

	if (local_scale >= 1) {
		*scale = local_scale;
		*skip = 1;
		*out_w = src_w * local_scale;
		*out_h = src_h * local_scale;
	} else {
		*scale = 0;
		local_skip = (src_w + video_w - 1) / video_w;
		if (((src_h + video_h - 1) / video_h) > local_skip)
			local_skip = (src_h + video_h - 1) / video_h;
		if (local_skip < 1)
			local_skip = 1;
		*skip = local_skip;
		*out_w = src_w / local_skip;
		*out_h = src_h / local_skip;
	}

	*off_x = (video_w - *out_w) / 2;
	*off_y = (video_h - *out_h) / 2;
	if (*off_x < 0)
		*off_x = 0;
	if (*off_y < 0)
		*off_y = 0;
}

static void driver_update_framebuffer(void)
{
	int src_w, src_h, src_pitch;
	int scale, skip, out_w, out_h, off_x, off_y;
	int x, y;

	if (!__fb_gfx || !framebuffer || !blitter)
		return;

	src_w = __fb_gfx->w;
	src_h = __fb_gfx->h;
	src_pitch = src_w * (int)sizeof(uint32_t);

	if (!ensure_scale_buffer((size_t)src_w * (size_t)src_h))
		return;

	blitter((unsigned char *)scale_buffer, src_pitch);
	get_viewport(&scale, &skip, &out_w, &out_h, &off_x, &off_y);

	memset(framebuffer, 0, (size_t)video_w * (size_t)video_h * sizeof(uint32_t));

	for (y = 0; y < out_h; ++y) {
		uint32_t *dst = ((uint32_t *)framebuffer) + ((off_y + y) * video_w) + off_x;
		uint32_t *src;

		if (scale >= 1)
			src = scale_buffer + ((y / scale) * src_w);
		else
			src = scale_buffer + ((y * skip) * src_w);

		for (x = 0; x < out_w; ++x) {
			if (scale >= 1)
				dst[x] = src[x / scale];
			else
				dst[x] = src[x * skip];
		}
	}

	XVideoFlushFB();
}

static int driver_init(char *title, int w, int h, int depth_arg, int refresh_rate, int flags)
{
	int mode_w, mode_h;
	VIDEO_MODE vm;

	(void)title;
	(void)depth_arg;

	if (flags & DRIVER_OPENGL)
		return -1;

	choose_video_mode(w, h, &mode_w, &mode_h);
	if (!XVideoSetMode(mode_w, mode_h, 32, refresh_rate))
		return -1;

	vm = XVideoGetMode();
	video_w = vm.width;
	video_h = vm.height;
	framebuffer = XVideoGetFB();
	blitter = fb_hGetBlitter(32, FALSE);
	if (!framebuffer || !blitter)
		return -1;

	quitting = FALSE;

	return 0;
}

static void driver_exit(void)
{
	quitting = TRUE;
	framebuffer = NULL;
	blitter = NULL;
	video_w = 0;
	video_h = 0;
	free(scale_buffer);
	scale_buffer = NULL;
	scale_buffer_size = 0;
}

static void driver_lock(void)
{
	/*
		The Xbox presenter is driven by SCREENUNLOCK and explicit page updates.
		There is no separate window thread to suspend here.
	*/
}

static void driver_unlock(void)
{
	if (!quitting)
		driver_update_framebuffer();
}

static void driver_set_palette(int index, int r, int g, int b)
{
	(void)index;
	(void)r;
	(void)g;
	(void)b;

	if (!quitting)
		driver_update_framebuffer();
}

static void driver_wait_vsync(void)
{
	XVideoWaitForVBlank();
}

static int driver_get_mouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	/*
		The stock Xbox controller ports do not provide a mouse cursor device.
		Controller state is exposed separately through GETXPAD.
	*/
	if (x) *x = -1;
	if (y) *y = -1;
	if (z) *z = -1;
	if (buttons) *buttons = -1;
	if (clip) *clip = -1;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

static void driver_set_mouse(int x, int y, int cursor, int clip)
{
	(void)x;
	(void)y;
	(void)cursor;
	(void)clip;
}

static int *driver_fetch_modes(int depth, int *size)
{
	VIDEO_MODE vm;
	void *p = NULL;
	int num = 0;
	int *modes = NULL, *new_modes;

	while (XVideoListModes(&vm, depth, 0, &p)) {
		++num;
		new_modes = realloc(modes, sizeof(int) * num);
		if (!new_modes) {
			*size = num - 1;
			return modes;
		}

		modes = new_modes;
		modes[num - 1] = SCREENLIST(vm.width, vm.height);
	}

	*size = num;
	return modes;
}

static void driver_poll_events(void)
{
	/*
		This backend has no window manager event source.  Controller input is a
		polled device API and is handled by fb_GfxGetXPad().
	*/
}

/* GFXDRIVER */
static const GFXDRIVER fb_gfxDriverXbox =
{
	"xbox",                  /* char *name; */
	driver_init,             /* int (*init)(char *title, int w, int h, int depth, int refresh_rate, int flags); */
	driver_exit,             /* void (*exit)(void); */
	driver_lock,             /* void (*lock)(void); */
	driver_unlock,           /* void (*unlock)(void); */
	driver_set_palette,      /* void (*set_palette)(int index, int r, int g, int b); */
	driver_wait_vsync,       /* void (*wait_vsync)(void); */
	driver_get_mouse,        /* int (*get_mouse)(int *x, int *y, int *z, int *buttons, int *clip); */
	driver_set_mouse,        /* void (*set_mouse)(int x, int y, int cursor, int clip); */
	NULL,                    /* void (*set_window_title)(char *title); */
	NULL,                    /* int (*set_window_pos)(int x, int y); */
	driver_fetch_modes,      /* int *(*fetch_modes)(int depth, int *size); */
	NULL,                    /* void (*flip)(void); */
	driver_poll_events,      /* void (*poll_events)(void); */
	NULL                     /* void (*update)(void); */
};

const GFXDRIVER *__fb_gfx_drivers_list[] = {
	&fb_gfxDriverXbox,
	NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	VIDEO_MODE vm;

	vm = XVideoGetMode();

	*width = vm.width;
	*height = vm.height;
	*depth = vm.bpp;
	*refresh = vm.refresh;
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
