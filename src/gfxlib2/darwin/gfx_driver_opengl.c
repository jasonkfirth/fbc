/*
 * FreeBASIC gfxlib2 Darwin backend
 * --------------------------------
 *
 * File: gfx_driver_opengl.c
 *
 * Purpose:
 *
 *     Expose an NSOpenGLView-backed desktop OpenGL driver to gfxlib2.
 *
 * Responsibilities:
 *
 *     - initialize the shared desktop OpenGL helper
 *     - provide ScreenGLProc symbol lookup
 *     - synchronize the optional OpenGL 2D bridge
 *     - flush the Cocoa OpenGL drawable
 *
 * This file intentionally does not contain Cocoa window construction,
 * event translation, or software framebuffer rendering.
 */

#include "../fb_gfx.h"
#include "../fb_gfx_gl.h"
#include "fb_gfx_darwin.h"

#if defined(HOST_DARWIN) && !defined(DISABLE_OPENGL)

#include <dlfcn.h>
#include <limits.h>

static FB_DYLIB fb_darwin_opengl_lib = NULL;

void *fb_hGL_GetProcAddress(const char *proc)
{
	if (!fb_darwin_opengl_lib || !proc || !proc[0])
		return NULL;

	return dlsym(fb_darwin_opengl_lib, proc);
}

static int driver_init(char *title, int w, int h, int depth,
	                   int refresh_rate, int flags)
{
	int result;

	if (!(flags & DRIVER_OPENGL))
		return -1;

	fb_hGL_NormalizeParameters(flags);
	__fb_gl_params.mode_2d = __fb_gl_params.init_mode_2d;
	__fb_gl_params.scale = __fb_gl_params.init_scale >= 1 ?
	                       __fb_gl_params.init_scale : 1;

	if ((__fb_gl_params.scale > 1) &&
	    ((w > INT_MAX / __fb_gl_params.scale) ||
	     (h > INT_MAX / __fb_gl_params.scale)))
		return -1;

	result = fb_hDarwinInit(title,
	                        w * __fb_gl_params.scale,
	                        h * __fb_gl_params.scale,
	                        depth, refresh_rate, flags);
	if (result != 0)
		return result;

	fb_darwin_opengl_lib = dlopen(
		"/System/Library/Frameworks/OpenGL.framework/OpenGL",
		RTLD_LAZY | RTLD_LOCAL
	);
	if (!fb_darwin_opengl_lib ||
	    (fb_hDarwinOpenGLMakeCurrent() != 0) ||
	    fb_hGL_Init(fb_darwin_opengl_lib, NULL))
	{
		fb_hDarwinExit();
		if (fb_darwin_opengl_lib)
			dlclose(fb_darwin_opengl_lib);
		fb_darwin_opengl_lib = NULL;
		return -1;
	}

	if (__fb_gl_params.num_samples > 0)
		__fb_gl.Enable(GL_MULTISAMPLE_ARB);

	if (__fb_gl_params.mode_2d != DRIVER_OGL_2D_NONE)
		fb_hGL_ScreenCreate();

	return 0;
}

static void driver_exit(void)
{
	fb_hDarwinExit();

	if (fb_darwin_opengl_lib)
		dlclose(fb_darwin_opengl_lib);
	fb_darwin_opengl_lib = NULL;
}

static void driver_lock(void)
{
	fb_hDarwinLock();
}

static void driver_unlock(void)
{
	if (__fb_gl_params.mode_2d == DRIVER_OGL_2D_AUTO_SYNC) {
		fb_hGL_SetupProjection();
		fb_hDarwinOpenGLSwapBuffers();
	}

	fb_hDarwinUnlock();
}

static void driver_flip(void)
{
	fb_hDarwinLock();

	if (__fb_gl_params.mode_2d == DRIVER_OGL_2D_MANUAL_SYNC)
		fb_hGL_SetupProjection();

	fb_hDarwinOpenGLSwapBuffers();
	fb_hDarwinUnlock();
}

const GFXDRIVER fb_gfxDriverDarwinOpenGL = {
	"DarwinOpenGL",
	driver_init,
	driver_exit,
	driver_lock,
	driver_unlock,
	fb_hGL_SetPalette,
	fb_hDarwinWaitVSync,
	fb_hDarwinGetMouse,
	NULL,
	NULL,
	fb_hDarwinSetMouse,
	fb_hDarwinSetWindowTitle,
	fb_hDarwinSetWindowPos,
	fb_hDarwinFetchModes,
	driver_flip,
	fb_hDarwinPollEvents,
	NULL
};

#endif

/* end of gfx_driver_opengl.c */
