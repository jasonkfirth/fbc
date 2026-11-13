#include "../../fb_gfx.h"
#include "fb_gfx_darwin.h"

#ifdef HOST_DARWIN

const GFXDRIVER *fb_gfx_drivers_list[] = {
#ifndef DISABLE_OPENGL
	&fb_gfxDriverDarwinOpenGL,
#endif
	&fb_gfxDriverDarwin,
	&fb_gfxDriverNull,
	NULL
};

void fb_hScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	if (fb_hDarwinScreenInfo(width, height, depth, refresh))
		return;

	*width = *height = *depth = *refresh = 0;
}

#endif
