/*
    FreeBASIC gfxlib2 Haiku backend
    --------------------------------

    File: gfx_driver_opengl.cpp

    Purpose:

        Expose a native BGLView OpenGL driver to gfxlib2.

    Responsibilities:

        - normalize gfxlib OpenGL options
        - initialize the shared desktop OpenGL helper
        - present BGLView buffers
        - provide ScreenGLProc symbol lookup

    This file intentionally does NOT contain window lifecycle management,
    input handling, or software framebuffer presentation.
*/

#if !defined(DISABLE_HAIKU) && !defined(DISABLE_OPENGL)

#include "../fb_gfx.h"
#include "../fb_gfx_gl.h"
#include "fb_gfx_haiku.h"

#include <dlfcn.h>
#include <limits.h>

static FB_DYLIB gl_library = NULL;

extern "C" void *fb_hGL_GetProcAddress(const char *proc)
{
    if (!gl_library || !proc || !proc[0])
        return NULL;

    return dlsym(gl_library, proc);
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

    /* BGLView does not expose a multisample pixel-format selector. */
    __fb_gl_params.num_samples = 0;

    result = fb_hHaikuInit(title,
                           w * __fb_gl_params.scale,
                           h * __fb_gl_params.scale,
                           depth, refresh_rate, flags);
    if (result != 0)
        return result;

    /* BGLView and the desktop OpenGL entry points are provided by libGL. */
    gl_library = dlopen("libGL.so", RTLD_LAZY | RTLD_LOCAL);
    if (!gl_library || fb_hGL_Init(gl_library, NULL)) {
        fb_hHaikuExit();
        if (gl_library)
            dlclose(gl_library);
        gl_library = NULL;
        return -1;
    }

    if (__fb_gl_params.mode_2d != DRIVER_OGL_2D_NONE)
        fb_hGL_ScreenCreate();

    return 0;
}

static void driver_exit(void)
{
    fb_hHaikuExit();

    if (gl_library)
        dlclose(gl_library);
    gl_library = NULL;
}

static void driver_lock(void)
{
    fb_hHaikuLock();
}

static void driver_unlock(void)
{
    if (__fb_gl_params.mode_2d == DRIVER_OGL_2D_AUTO_SYNC)
    {
        fb_hGL_SetupProjection();
        fb_hHaikuOpenGLFlip();
    }

    fb_hHaikuUnlock();
}

void fb_hHaikuOpenGLFlip(void)
{
    if (g_gl_view)
        g_gl_view->SwapBuffers();
}

static void driver_flip(void)
{
    fb_hHaikuLock();

    if (__fb_gl_params.mode_2d == DRIVER_OGL_2D_MANUAL_SYNC)
        fb_hGL_SetupProjection();

    fb_hHaikuOpenGLFlip();
    fb_hHaikuUnlock();
}

extern "C" const GFXDRIVER fb_gfxDriverHaikuOpenGL =
{
    (char*)"HaikuOpenGL",
    driver_init,
    driver_exit,
    driver_lock,
    driver_unlock,
    fb_hGL_SetPalette,
    fb_hHaikuWaitVSync,
    fb_hHaikuGetMouse,
    fb_hHaikuGetTouchCount,
    fb_hHaikuGetTouch,
    fb_hHaikuSetMouse,
    fb_hHaikuSetWindowTitle,
    fb_hHaikuSetWindowPos,
    fb_hHaikuFetchModes,
    driver_flip,
    fb_hHaikuPollEvents,
    NULL,
    NULL
};

#endif

/* end of gfx_driver_opengl.cpp */
