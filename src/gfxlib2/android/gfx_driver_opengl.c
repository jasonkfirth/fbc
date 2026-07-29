/*
 * FreeBASIC gfxlib2
 * -----------------
 *
 * File: gfx_driver_opengl.c
 *
 * Purpose:
 *
 *     Provide an EGL-backed OpenGL ES context for Android NativeActivity
 *     windows.
 *
 * Responsibilities:
 *
 *     - EGL display, config, context, and window-surface ownership
 *     - NativeActivity window replacement and lifecycle recovery
 *     - buffer swapping and ScreenGLProc symbol lookup
 *
 * This file intentionally does not contain software framebuffer rendering,
 * input handling, or NativeActivity callback dispatch.
 */

#include "../fb_gfx_gl.h"
#include "fb_gfx_android.h"

#ifndef DISABLE_OPENGL

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct FB_ANDROID_EGL_STATE
{
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
	ANativeWindow *surface_window;
	unsigned surface_generation;
	void *gles_library;
} FB_ANDROID_EGL_STATE;

static FB_ANDROID_EGL_STATE fb_android_egl =
{
	.display = EGL_NO_DISPLAY,
	.config = NULL,
	.context = EGL_NO_CONTEXT,
	.surface = EGL_NO_SURFACE,
	.surface_window = NULL,
	.surface_generation = 0,
	.gles_library = NULL
};

static void android_egl_log_failure(const char *operation)
{
	char message[128];

	snprintf(message, sizeof(message), "%s failed (EGL error 0x%04x)",
	         operation, (unsigned int)eglGetError());
	fb_hAndroidLog(message);
}

static void android_egl_destroy_surface(void)
{
	if (fb_android_egl.display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(fb_android_egl.display, EGL_NO_SURFACE,
		               EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (fb_android_egl.surface != EGL_NO_SURFACE)
			eglDestroySurface(fb_android_egl.display, fb_android_egl.surface);
	}

	fb_android_egl.surface = EGL_NO_SURFACE;

	if (fb_android_egl.surface_window)
		ANativeWindow_release(fb_android_egl.surface_window);

	fb_android_egl.surface_window = NULL;
	fb_android_egl.surface_generation = 0;
}

static int android_egl_make_surface_current(void)
{
	ANativeWindow *window;
	unsigned generation;
	int renderable;
	EGLint native_format;

	window = fb_hAndroidAcquireWindow(&generation, &renderable);
	if (!window || !renderable)
	{
		if (window)
			ANativeWindow_release(window);
		android_egl_destroy_surface();
		return -1;
	}

	if ((fb_android_egl.surface != EGL_NO_SURFACE) &&
	    (fb_android_egl.surface_window == window) &&
	    (fb_android_egl.surface_generation == generation))
	{
		ANativeWindow_release(window);
		if (!eglMakeCurrent(fb_android_egl.display, fb_android_egl.surface,
		                    fb_android_egl.surface, fb_android_egl.context))
		{
			android_egl_log_failure("eglMakeCurrent");
			android_egl_destroy_surface();
			return -1;
		}
		return 0;
	}

	android_egl_destroy_surface();

	/*
	 * EGL_NATIVE_VISUAL_ID is the buffer format selected by the config.
	 * Setting it before eglCreateWindowSurface() keeps the native window and
	 * EGL producer in agreement about buffer layout.
	 */
	if (!eglGetConfigAttrib(fb_android_egl.display, fb_android_egl.config,
	                        EGL_NATIVE_VISUAL_ID, &native_format))
	{
		android_egl_log_failure("eglGetConfigAttrib(EGL_NATIVE_VISUAL_ID)");
		ANativeWindow_release(window);
		return -1;
	}
	if (ANativeWindow_setBuffersGeometry(window, 0, 0, native_format) != 0)
	{
		fb_hAndroidLog("ANativeWindow_setBuffersGeometry failed");
		ANativeWindow_release(window);
		return -1;
	}

	fb_android_egl.surface = eglCreateWindowSurface(fb_android_egl.display,
	                                                fb_android_egl.config,
	                                                window, NULL);
	if (fb_android_egl.surface == EGL_NO_SURFACE)
	{
		android_egl_log_failure("eglCreateWindowSurface");
		ANativeWindow_release(window);
		return -1;
	}

	if (!eglMakeCurrent(fb_android_egl.display, fb_android_egl.surface,
	                    fb_android_egl.surface, fb_android_egl.context))
	{
		android_egl_log_failure("eglMakeCurrent");
		eglDestroySurface(fb_android_egl.display, fb_android_egl.surface);
		fb_android_egl.surface = EGL_NO_SURFACE;
		ANativeWindow_release(window);
		return -1;
	}

	fb_android_egl.surface_window = window;
	fb_android_egl.surface_generation = generation;
	return 0;
}

static int android_egl_choose_config(void)
{
	EGLint attributes[] =
	{
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 0,
		EGL_GREEN_SIZE, 0,
		EGL_BLUE_SIZE, 0,
		EGL_ALPHA_SIZE, 0,
		EGL_DEPTH_SIZE, 0,
		EGL_STENCIL_SIZE, 0,
		EGL_SAMPLE_BUFFERS, 0,
		EGL_SAMPLES, 0,
		EGL_NONE
	};
	EGLint count;
	EGLint samples;

	attributes[5] = __fb_gl_params.color_red_bits;
	attributes[7] = __fb_gl_params.color_green_bits;
	attributes[9] = __fb_gl_params.color_blue_bits;
	attributes[11] = __fb_gl_params.color_alpha_bits;
	attributes[13] = __fb_gl_params.depth_bits;
	attributes[15] = __fb_gl_params.stencil_bits;

	samples = __fb_gl_params.num_samples;
	if (samples < 0)
		samples = 0;

	for (;;)
	{
		attributes[17] = samples > 0 ? 1 : 0;
		attributes[19] = samples;
		count = 0;

		if (eglChooseConfig(fb_android_egl.display, attributes,
		                    &fb_android_egl.config, 1, &count) && (count > 0))
		{
			__fb_gl_params.num_samples = samples;
			return 0;
		}

		if (samples == 0)
			break;

		samples -= 2;
		if (samples < 0)
			samples = 0;
	}

	return -1;
}

int fb_hAndroidOpenGLInit(char *title, int w, int h, int depth, int refresh_rate,
                         int flags, int require_api26)
{
	static const EGLint context_attributes[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLint major;
	EGLint minor;
	int result;

	if (!(flags & DRIVER_OPENGL))
		return -1;

	fb_hGL_NormalizeParameters(flags);

	/* gfxlib2's framebuffer-to-texture bridge requires desktop OpenGL. */
	if ((__fb_gl_params.init_mode_2d != DRIVER_OGL_2D_NONE) ||
	    (__fb_gl_params.accum_bits > 0))
		return -1;

	result = fb_hAndroidInit(title, w, h, depth, refresh_rate, flags, require_api26);
	if (result != 0)
		return result;

	fb_android_egl.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (fb_android_egl.display == EGL_NO_DISPLAY) {
		android_egl_log_failure("eglGetDisplay");
		goto fail;
	}
	if (!eglInitialize(fb_android_egl.display, &major, &minor)) {
		android_egl_log_failure("eglInitialize");
		goto fail;
	}
	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		android_egl_log_failure("eglBindAPI");
		goto fail;
	}
	if (android_egl_choose_config() != 0) {
		android_egl_log_failure("eglChooseConfig");
		goto fail;
	}

	fb_android_egl.context = eglCreateContext(fb_android_egl.display,
	                                         fb_android_egl.config,
	                                         EGL_NO_CONTEXT,
	                                         context_attributes);
	if (fb_android_egl.context == EGL_NO_CONTEXT)
	{
		android_egl_log_failure("eglCreateContext");
		goto fail;
	}

	if (android_egl_make_surface_current() != 0)
		goto fail;

	if (!eglSwapInterval(fb_android_egl.display, 1))
		android_egl_log_failure("eglSwapInterval");
	fb_android_egl.gles_library = dlopen("libGLESv2.so", RTLD_LAZY | RTLD_LOCAL);
	{
		const GLubyte *extensions = glGetString(GL_EXTENSIONS);

		if (extensions) {
			strncpy(__fb_gl_extensions, (const char *)extensions,
			        FBGL_EXTENSIONS_STRING_SIZE - 1);
			__fb_gl_extensions[FBGL_EXTENSIONS_STRING_SIZE - 1] = '\0';
		} else {
			__fb_gl_extensions[0] = '\0';
		}
	}
	__fb_gl_params.mode_2d = DRIVER_OGL_2D_NONE;
	__fb_gl_params.scale = 1;
	return 0;

fail:
	fb_hAndroidOpenGLExit();
	return -1;
}

void fb_hAndroidOpenGLExit(void)
{
	android_egl_destroy_surface();

	if ((fb_android_egl.display != EGL_NO_DISPLAY) &&
	    (fb_android_egl.context != EGL_NO_CONTEXT))
	{
		eglDestroyContext(fb_android_egl.display, fb_android_egl.context);
	}
	fb_android_egl.context = EGL_NO_CONTEXT;

	if (fb_android_egl.display != EGL_NO_DISPLAY)
		eglTerminate(fb_android_egl.display);
	fb_android_egl.display = EGL_NO_DISPLAY;
	fb_android_egl.config = NULL;

	if (fb_android_egl.gles_library)
		dlclose(fb_android_egl.gles_library);
	fb_android_egl.gles_library = NULL;
	__fb_gl_extensions[0] = '\0';

	fb_hAndroidExit();
}

void fb_hAndroidOpenGLFlip(void)
{
	if (android_egl_make_surface_current() != 0)
		return;

	if (!eglSwapBuffers(fb_android_egl.display, fb_android_egl.surface))
	{
		/* A replacement NativeActivity window is handled on the next flip. */
		android_egl_log_failure("eglSwapBuffers");
		android_egl_destroy_surface();
	}
}

void *fb_hGL_GetProcAddress(const char *proc)
{
	void *address;
	__eglMustCastToProperFunctionPointerType egl_address;

	if (!proc || !proc[0])
		return NULL;

	address = fb_android_egl.gles_library ?
	          dlsym(fb_android_egl.gles_library, proc) : NULL;
	if (address)
		return address;

	egl_address = eglGetProcAddress(proc);
	address = NULL;
	if (sizeof(address) == sizeof(egl_address))
		memcpy(&address, &egl_address, sizeof(address));

	return address;
}

ssize_t fb_hAndroidOpenGLDisplayHandle(void)
{
	return (ssize_t)(intptr_t)fb_android_egl.display;
}

#else

int fb_hAndroidOpenGLInit(char *title, int w, int h, int depth, int refresh_rate,
                         int flags, int require_api26)
{
	(void)title;
	(void)w;
	(void)h;
	(void)depth;
	(void)refresh_rate;
	(void)flags;
	(void)require_api26;
	return -1;
}

void fb_hAndroidOpenGLExit(void)
{
	fb_hAndroidExit();
}

void fb_hAndroidOpenGLFlip(void)
{
}

ssize_t fb_hAndroidOpenGLDisplayHandle(void)
{
	return 0;
}

#endif

/* end of gfx_driver_opengl.c */
