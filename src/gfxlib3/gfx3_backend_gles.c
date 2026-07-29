/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_gles.c

    Purpose:

        Provide the OpenGL ES backend contract on targets without the Android
        GLES implementation.

    Responsibilities:

        - expose the stable GLES backend vtable
        - reject GLES initialization on unsupported targets

    This file intentionally does NOT contain:

        - EGL or OpenGL ES system calls
        - Android window management
        - software rendering fallbacks
*/

#include "gfx3_backend_gles.h"

static int gles_probe(FB_GFX3_BACKEND_CAPS *caps)
{
	if (caps != NULL)
		memset(caps, 0, sizeof(*caps));
	return FB_GFX3_UNSUPPORTED;
}

static int gles_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	(void)backend;
	(void)config;
	return FB_GFX3_UNSUPPORTED;
}

static void gles_shutdown(FB_GFX3_BACKEND *backend)
{
	if (backend != NULL)
		backend->state = NULL;
}

static int gles_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	(void)backend;
	(void)commands;
	(void)count;
	(void)submitted_sequence;
	return FB_GFX3_UNSUPPORTED;
}

static uint64_t gles_completed_sequence(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return 0;
}

static int gles_wait_sequence(FB_GFX3_BACKEND *backend, uint64_t sequence)
{
	(void)backend;
	(void)sequence;
	return FB_GFX3_UNSUPPORTED;
}

static int gles_wait_idle(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return FB_GFX3_UNSUPPORTED;
}

static void *gles_get_opengl_proc(FB_GFX3_BACKEND *backend,
	const char *name)
{
	(void)backend;
	(void)name;
	return NULL;
}

const FB_GFX3_BACKEND_VTABLE __fb_gfx3_backend_gles = {
	FB_GFX3_BACKEND_ABI_VERSION,
	"OpenGL ES 3.0",
	gles_probe,
	gles_init,
	gles_shutdown,
	gles_execute,
	gles_completed_sequence,
	gles_wait_sequence,
	gles_wait_idle,
	gles_get_opengl_proc
};

/* end of gfx3_backend_gles.c */
