#include "../fb_gfx.h"
#include "fb_gfx_android.h"

static int modern_init(char *title, int w, int h, int depth, int refresh_rate, int flags)
{
	if (flags & DRIVER_OPENGL)
		return -1;

	return fb_hAndroidInit(title, w, h, depth, refresh_rate, flags, 1);
}

static int legacy_init(char *title, int w, int h, int depth, int refresh_rate, int flags)
{
	if (flags & DRIVER_OPENGL)
		return -1;

	return fb_hAndroidInit(title, w, h, depth, refresh_rate, flags, 0);
}

static int opengles_modern_init(char *title, int w, int h, int depth, int refresh_rate, int flags)
{
	return fb_hAndroidOpenGLInit(title, w, h, depth, refresh_rate, flags, 1);
}

static int opengles_legacy_init(char *title, int w, int h, int depth, int refresh_rate, int flags)
{
	return fb_hAndroidOpenGLInit(title, w, h, depth, refresh_rate, flags, 0);
}

const GFXDRIVER fb_gfxDriverAndroidModern =
{
	"AndroidModern",
	modern_init,
	fb_hAndroidExit,
	fb_hAndroidLock,
	fb_hAndroidUnlock,
	fb_hAndroidSetPalette,
	fb_hAndroidWaitVSync,
	fb_hAndroidGetMouse,
	fb_hAndroidGetTouchCount,
	fb_hAndroidGetTouch,
	fb_hAndroidSetMouse,
	fb_hAndroidSetWindowTitle,
	fb_hAndroidSetWindowPos,
	fb_hAndroidFetchModes,
	NULL,
	fb_hAndroidPollEvents,
	fb_hAndroidUpdate,
	NULL
};

const GFXDRIVER fb_gfxDriverAndroidLegacy =
{
	"AndroidLegacy",
	legacy_init,
	fb_hAndroidExit,
	fb_hAndroidLock,
	fb_hAndroidUnlock,
	fb_hAndroidSetPalette,
	fb_hAndroidWaitVSync,
	fb_hAndroidGetMouse,
	fb_hAndroidGetTouchCount,
	fb_hAndroidGetTouch,
	fb_hAndroidSetMouse,
	fb_hAndroidSetWindowTitle,
	fb_hAndroidSetWindowPos,
	fb_hAndroidFetchModes,
	NULL,
	fb_hAndroidPollEvents,
	fb_hAndroidUpdate,
	NULL
};

const GFXDRIVER fb_gfxDriverAndroidOpenGLESModern =
{
	"AndroidOpenGLESModern",
	opengles_modern_init,
	fb_hAndroidOpenGLExit,
	fb_hAndroidLock,
	fb_hAndroidUnlock,
	fb_hAndroidSetPalette,
	fb_hAndroidWaitVSync,
	fb_hAndroidGetMouse,
	fb_hAndroidGetTouchCount,
	fb_hAndroidGetTouch,
	fb_hAndroidSetMouse,
	fb_hAndroidSetWindowTitle,
	fb_hAndroidSetWindowPos,
	fb_hAndroidFetchModes,
	fb_hAndroidOpenGLFlip,
	fb_hAndroidPollEvents,
	NULL,
	NULL
};

const GFXDRIVER fb_gfxDriverAndroidOpenGLESLegacy =
{
	"AndroidOpenGLESLegacy",
	opengles_legacy_init,
	fb_hAndroidOpenGLExit,
	fb_hAndroidLock,
	fb_hAndroidUnlock,
	fb_hAndroidSetPalette,
	fb_hAndroidWaitVSync,
	fb_hAndroidGetMouse,
	fb_hAndroidGetTouchCount,
	fb_hAndroidGetTouch,
	fb_hAndroidSetMouse,
	fb_hAndroidSetWindowTitle,
	fb_hAndroidSetWindowPos,
	fb_hAndroidFetchModes,
	fb_hAndroidOpenGLFlip,
	fb_hAndroidPollEvents,
	NULL,
	NULL
};

const GFXDRIVER *__fb_gfx_drivers_list[] =
{
	&fb_gfxDriverAndroidOpenGLESModern,
	&fb_gfxDriverAndroidOpenGLESLegacy,
	&fb_gfxDriverAndroidModern,
	&fb_gfxDriverAndroidLegacy,
	NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	fb_hAndroidScreenInfo(width, height, depth, refresh);
}
