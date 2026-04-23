/* Minimal Darwin OpenGL symbol lookup support */

#include "../fb_gfx_gl.h"

#ifndef DISABLE_OPENGL

#include <dlfcn.h>

static void *fb_darwin_opengl_lib = NULL;
static int fb_darwin_opengl_tried = 0;

void *fb_hGL_GetProcAddress(const char *proc)
{
	if (!fb_darwin_opengl_tried) {
		fb_darwin_opengl_tried = 1;
		fb_darwin_opengl_lib = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY | RTLD_LOCAL);
	}

	if (!fb_darwin_opengl_lib)
		return NULL;

	return dlsym(fb_darwin_opengl_lib, proc);
}

#endif /* !DISABLE_OPENGL */
