/* xbox fbgfx driver */

#include "../fb_gfx.h"
#include <hal/xbox.h>
#include <hal/video.h>

#define SCREENLIST(w, h) ((h) | (w) << 16)

static BLITTER *blitter;
static void *framebuffer;
static volatile int quitting;

static void driver_update_framebuffer(void)
{
	if (!__fb_gfx || !framebuffer)
		return;

	if (blitter)
		blitter(framebuffer, __fb_gfx->w);
	else
		memcpy(framebuffer, __fb_gfx->framebuffer, __fb_gfx->pitch * __fb_gfx->h);

	XVideoFlushFB();
}

static int driver_init(char *title, int w, int h, int depth_arg, int refresh_rate, int flags)
{
	int depth = MAX(8, depth_arg);

	if (depth == 8) {
		depth = 24;
		blitter = fb_hGetBlitter(32, FALSE);
	} else {
		blitter = NULL;
	}

	if (flags & DRIVER_OPENGL)
		return -1;

	if (!XVideoSetMode(w, h, depth == 32 ? 24 : depth, refresh_rate))
		return -1;

	framebuffer = XVideoGetFB();

	quitting = FALSE;

	//XInput_Init();

	return 0;
}

static void driver_exit(void)
{
	quitting = TRUE;

	//XInput_Quit();

	/* !!!FIXME!!! */
}

static void driver_lock(void)
{
	/* !!!WRITEME!!! */
}

static void driver_unlock(void)
{
	if (!quitting)
		driver_update_framebuffer();
}

static void driver_set_palette(int index, int r, int g, int b)
{
	/* !!!WRITEME!!! */
}

static void driver_wait_vsync(void)
{
	/* !!!WRITEME!!! */
}

static int driver_get_mouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	/* !!!WRITEME!!! */
	if (x) *x = -1;
	if (y) *y = -1;
	if (z) *z = -1;
	if (buttons) *buttons = -1;
	if (clip) *clip = -1;
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}

static void driver_set_mouse(int x, int y, int cursor, int clip)
{
	/* !!!WRITEME!!! */
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
	/* !!!WRITEME!!! */
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
	/* !!!FIXME!!! */
	VIDEO_MODE vm;

	vm = XVideoGetMode();

	*width = vm.width;
	*height = vm.height;
	*depth = vm.bpp;
	*refresh = vm.refresh;
}

FBCALL int fb_GfxGetJoystick(int id, ssize_t *buttons, float *a1, float *a2, float *a3, float *a4, float *a5, float *a6, float *a7, float *a8)
{
	(void)id;
	FB_GRAPHICS_LOCK( );

	*buttons = -1;
	*a1 = *a2 = *a3 = *a4 = *a5 = *a6 = *a7 = *a8 = -1000.0;

	FB_GRAPHICS_UNLOCK( );
	return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
}
