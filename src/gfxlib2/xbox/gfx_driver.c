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

        - window management
        - audio or lifecycle handling
        - controller-to-keyboard input synthesis

    Platform notes:

        The original Xbox does not have a PC keyboard in the normal console
        setup.  Controller state is reported through GETXPAD so applications
        can decide how a pad should interact with their own menus and game
        controls.  The graphics driver deliberately does not write into the
        keyboard state or queue synthetic INKEY$ events.
*/

#include "../fb_gfx.h"
#include <hal/xbox.h>
#include <hal/video.h>
#include <winapi/handleapi.h>
#include <winapi/processthreadsapi.h>
#ifdef WAIT_TIMEOUT
#undef WAIT_TIMEOUT
#endif
#include <winapi/synchapi.h>
#include <winapi/sysinfoapi.h>
#include <stdint.h>

#define SCREENLIST(w, h) ((h) | (w) << 16)
#define XBOX_DEFAULT_W 640
#define XBOX_DEFAULT_H 480
#define XBOX_CRTC_STATUS 0x006013DA
#define XBOX_CRTC_STATUS_VBLANK 0x08
#define XBOX_VBLANK_TIMEOUT_MS 50
#define XBOX_PRESENT_INTERVAL_MS 10

/*
	Xbox scanout must never be used as a work surface.

	Copying into the framebuffer currently being scanned can create a black
	tear that only exists for a fraction of a frame.  The backend therefore
	composes into normal memory, copies the completed image into a non-visible
	hardware framebuffer, then switches scanout during vblank.
*/
#define XBOX_SCAN_BUFFERS 3

static BLITTER *blitter;
static void *framebuffer;
static uint32_t *scale_buffer;
static size_t scale_buffer_size;
static uint32_t *present_buffer;
static size_t present_buffer_size;
/*
	XVideoSetFB() programs the CRTC start register.  The buffers below are
	normal kernel virtual addresses for CPU writes, but the register needs
	the corresponding low physical address returned by MmGetPhysicalAddress().
*/
static uint32_t *scan_buffer[XBOX_SCAN_BUFFERS];
static uintptr_t scan_buffer_physical[XBOX_SCAN_BUFFERS];
static int front_scan_buffer;
static uintptr_t framebuffer_physical;
static size_t video_framebuffer_size;
static CRITICAL_SECTION update_lock;
static HANDLE presenter_thread;
static int update_lock_ready;
static int video_w;
static int video_h;
static volatile int quitting;

static void wait_for_vblank_edge(void)
{
	DWORD start;

	/*
		XVideoWaitForVBlank() waits through an interrupt event.  That is fine
		for throttling, but a delayed wakeup can put XVideoSetFB() just after
		the blanking interval.  Poll the CRT status bit here so framebuffer
		address changes happen at the start of vertical blanking.
	*/
	start = GetTickCount();
	while (VIDEOREG8(XBOX_CRTC_STATUS) & XBOX_CRTC_STATUS_VBLANK) {
		if ((DWORD)(GetTickCount() - start) > XBOX_VBLANK_TIMEOUT_MS)
			return;
	}

	start = GetTickCount();
	while (!(VIDEOREG8(XBOX_CRTC_STATUS) & XBOX_CRTC_STATUS_VBLANK)) {
		if ((DWORD)(GetTickCount() - start) > XBOX_VBLANK_TIMEOUT_MS)
			return;
	}
}

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

static int ensure_present_buffer(size_t pixels)
{
	uint32_t *new_buffer;

	if (present_buffer_size >= pixels)
		return TRUE;

	new_buffer = realloc(present_buffer, pixels * sizeof(uint32_t));
	if (!new_buffer)
		return FALSE;

	present_buffer = new_buffer;
	present_buffer_size = pixels;
	return TRUE;
}

static int count_dirty_lines(void)
{
	int y, count = 0;

	if (!__fb_gfx || !__fb_gfx->dirty)
		return 0;

	for (y = 0; y < __fb_gfx->h; ++y) {
		if (__fb_gfx->dirty[y])
			++count;
	}

	return count;
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
#ifndef GFXLIB_NEVERSCALE
	int local_scale;
	int local_skip;
#endif

#ifdef GFXLIB_NEVERSCALE
	*scale = 1;
	*skip = 1;
	*out_w = src_w;
	*out_h = src_h;
	if (*out_w > video_w)
		*out_w = video_w;
	if (*out_h > video_h)
		*out_h = video_h;
	*off_x = (video_w - *out_w) / 2;
	*off_y = (video_h - *out_h) / 2;
	if (*off_x < 0)
		*off_x = 0;
	if (*off_y < 0)
		*off_y = 0;
	return;
#endif

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
	int back_scan_buffer;
	int dirty_lines;
	int x, y;

	if (!__fb_gfx || !framebuffer || !blitter || !scan_buffer[0])
		return;

	dirty_lines = count_dirty_lines();
	if (dirty_lines == 0)
		return;

	src_w = __fb_gfx->w;
	src_h = __fb_gfx->h;
	src_pitch = src_w * (int)sizeof(uint32_t);

	if (!ensure_scale_buffer((size_t)src_w * (size_t)src_h))
		return;

	if (!ensure_present_buffer((size_t)video_w * (size_t)video_h))
		return;

	blitter((unsigned char *)scale_buffer, src_pitch);
	get_viewport(&scale, &skip, &out_w, &out_h, &off_x, &off_y);

	for (y = 0; y < out_h; ++y) {
		uint32_t *dst = present_buffer + ((off_y + y) * video_w) + off_x;
		uint32_t *src;
		int src_y;

		if (scale >= 1) {
			src_y = y / scale;
		} else {
			src_y = y * skip;
			if (src_y >= src_h)
				src_y = src_h - 1;
		}

		if (!__fb_gfx->dirty[src_y])
			continue;

		src = scale_buffer + (src_y * src_w);

		for (x = 0; x < out_w; ++x) {
			if (scale >= 1)
				dst[x] = src[x / scale];
			else
				dst[x] = src[x * skip];
		}
	}

	/*
		Xbox video framebuffers are scanout surfaces.  Updating the currently
		visible surface can expose a partially redrawn BASIC frame, which
		looks like a black/text/black flash in programs that redraw with CLS
		followed by many primitives.

		Keep the scaled image in normal memory, copy one complete composed
		frame to the non-visible hardware framebuffer, and then switch scanout
		during vblank.  This is not gfxlib page flipping; it is only a backend
		scanout buffer swap that keeps the display from seeing the middle of a
		software redraw.
	*/
	back_scan_buffer = front_scan_buffer + 1;
	if (back_scan_buffer >= XBOX_SCAN_BUFFERS)
		back_scan_buffer = 0;

	fb_hMemCpy(scan_buffer[back_scan_buffer], present_buffer, video_framebuffer_size);
	XVideoFlushFB();
	wait_for_vblank_edge();
	XVideoSetFB((unsigned char *)scan_buffer_physical[back_scan_buffer]);

	/*
		Some video hardware latches the CRTC start address at vblank.  Wait
		one more blanking interval before allowing the previous scanout buffer
		to become the writable back buffer again.  Without this, a fast
		visible-page redraw can reuse a buffer that the TV is still scanning,
		which appears as black tearing between frames.
	*/
	wait_for_vblank_edge();
	front_scan_buffer = back_scan_buffer;
	fb_hMemSet(__fb_gfx->dirty, FALSE, __fb_gfx->h);
}

static DWORD WINAPI presenter_thread_proc(LPVOID param)
{
	(void)param;

	while (!quitting) {
		Sleep(XBOX_PRESENT_INTERVAL_MS);

		if (quitting)
			break;

		EnterCriticalSection(&update_lock);
		if (!quitting)
			driver_update_framebuffer();
		LeaveCriticalSection(&update_lock);
	}

	return 0;
}

static int driver_init(char *title, int w, int h, int depth_arg, int refresh_rate, int flags)
{
	int mode_w, mode_h;
	int i;
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
	video_framebuffer_size = (size_t)video_w * (size_t)video_h * sizeof(uint32_t);
	framebuffer = XVideoGetFB();
	blitter = fb_hGetBlitter(32, FALSE);
	if (!framebuffer || !blitter)
		return -1;
	framebuffer_physical = MmGetPhysicalAddress(framebuffer);

	for (i = 0; i < XBOX_SCAN_BUFFERS; ++i) {
		scan_buffer[i] = (uint32_t *)MmAllocateContiguousMemoryEx(video_framebuffer_size,
		                                                          0x00000000,
		                                                          0x7FFFFFFF,
		                                                          0x1000,
		                                                          PAGE_READWRITE | PAGE_WRITECOMBINE);
		if (!scan_buffer[i]) {
			while (i > 0) {
				--i;
				MmFreeContiguousMemory(scan_buffer[i]);
				scan_buffer[i] = NULL;
			}
			return -1;
		}

		scan_buffer_physical[i] = MmGetPhysicalAddress(scan_buffer[i]);
	}
	front_scan_buffer = 0;

	if (!ensure_present_buffer((size_t)video_w * (size_t)video_h)) {
		for (i = 0; i < XBOX_SCAN_BUFFERS; ++i) {
			MmFreeContiguousMemory(scan_buffer[i]);
			scan_buffer[i] = NULL;
		}
		return -1;
	}

	memset(present_buffer, 0, video_framebuffer_size);
	for (i = 0; i < XBOX_SCAN_BUFFERS; ++i)
		memset(scan_buffer[i], 0, video_framebuffer_size);
	XVideoFlushFB();
	XVideoSetFB((unsigned char *)scan_buffer_physical[front_scan_buffer]);

	InitializeCriticalSection(&update_lock);
	update_lock_ready = TRUE;
	quitting = FALSE;
	presenter_thread = CreateThread(NULL, 0, presenter_thread_proc, NULL, 0, NULL);
	if (!presenter_thread) {
		quitting = TRUE;
		DeleteCriticalSection(&update_lock);
		update_lock_ready = FALSE;
		return -1;
	}

	return 0;
}

static void driver_exit(void)
{
	int i;

	quitting = TRUE;
	if (presenter_thread) {
		WaitForSingleObject(presenter_thread, INFINITE);
		CloseHandle(presenter_thread);
		presenter_thread = NULL;
	}
	if (update_lock_ready) {
		DeleteCriticalSection(&update_lock);
		update_lock_ready = FALSE;
	}
	if (framebuffer)
		XVideoSetFB((unsigned char *)framebuffer_physical);
	framebuffer = NULL;
	framebuffer_physical = 0;
	blitter = NULL;
	video_w = 0;
	video_h = 0;
	video_framebuffer_size = 0;
	free(scale_buffer);
	scale_buffer = NULL;
	scale_buffer_size = 0;
	free(present_buffer);
	present_buffer = NULL;
	present_buffer_size = 0;
	for (i = 0; i < XBOX_SCAN_BUFFERS; ++i) {
		if (scan_buffer[i])
			MmFreeContiguousMemory(scan_buffer[i]);
		scan_buffer[i] = NULL;
		scan_buffer_physical[i] = 0;
	}
	front_scan_buffer = 0;
}

static void driver_lock(void)
{
	/*
		Match the Win32 drivers: primitive drawing and presentation share a
		driver-owned update lock.  This prevents the presenter thread from
		copying a framebuffer line while a primitive is modifying it.
	*/
	if (update_lock_ready)
		EnterCriticalSection(&update_lock);
}

static void driver_unlock(void)
{
	/*
		Unlock only releases the presenter.  It does not publish a frame.
		Win32 follows the same split: drawing unlocks synchronization, and the
		driver's paint/update path decides when the display surface changes.
	*/
	if (update_lock_ready)
		LeaveCriticalSection(&update_lock);
}

static void driver_update(void)
{
	/*
		The presenter thread polls dirty state at display cadence.  Keeping
		this hook passive avoids turning tight SLEEP 1 loops into hundreds of
		synchronous presents per second.
	*/
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
	wait_for_vblank_edge();
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
	NULL,                    /* int (*get_touch_count)(void); */
	NULL,                    /* int (*get_touch)(int index, int *x, int *y, int *id); */
	driver_set_mouse,        /* void (*set_mouse)(int x, int y, int cursor, int clip); */
	NULL,                    /* void (*set_window_title)(char *title); */
	NULL,                    /* int (*set_window_pos)(int x, int y); */
	driver_fetch_modes,      /* int *(*fetch_modes)(int depth, int *size); */
	NULL,                    /* void (*flip)(void); */
	NULL,                    /* void (*poll_events)(void); */
	driver_update,           /* void (*update)(void); */
	NULL                     /* int (*resize)(int width, int height); */
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

ssize_t fb_hGetWindowHandle(void)
{
	/*
		SCREENCONTROL exposes native window/display handles on targets that
		actually have a host window system.  The Xbox backend owns the video
		framebuffer directly through nxdk, so there is no stable handle that a
		FreeBASIC program could use outside gfxlib.
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
