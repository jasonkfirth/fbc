/*
 * FreeBASIC gfxlib2: gfx_screeninfo.c
 * Purpose: Report active graphics dimensions and pixel format.
 * Responsibilities: driver selection, pages and mode queries.
 * This file intentionally does NOT contain drawing algorithms or native presentation.
 */

/* Function to get informations about current gfx mode. */

#include "../fb_gfx.h"

FBCALL void fb_GfxScreenInfo
	(
		ssize_t *width,
		ssize_t *height,
		ssize_t *depth,
		ssize_t *bpp,
		ssize_t *pitch,
		ssize_t *refresh,
		FBSTRING_ACCESS *driver
	)
{
	const char *name;
	size_t name_len;

	FB_GRAPHICS_LOCK( );
	fb_hApplyPendingResize();

	if (!fb_gfx) {
		name = "";
		fb_hScreenInfo(width, height, depth, refresh);
		*bpp = *pitch = 0;
	} else {
		name = fb_gfx->driver->name;
		*width = fb_gfx->w;
		*height = fb_gfx->h;
		*depth = fb_gfx->depth;
		*bpp = fb_gfx->bpp;
		*pitch = fb_gfx->pitch;
		*refresh = fb_gfx->refresh_rate;
	}

	if (fb_hStrDelTemp(driver)) {
		name_len = strlen(name);
		fb_hStrRealloc(driver, name_len, FB_FALSE);
		memcpy(driver->data, name, name_len + 1);
	}

	FB_GRAPHICS_UNLOCK( );
}

FBCALL void fb_GfxScreenInfo32
	(
		int *width,
		int *height,
		int *depth,
		int *bpp,
		int *pitch,
		int *refresh,
		FBSTRING_ACCESS *driver
	)
{
	ssize_t w, h, d, b, p, r;
	fb_GfxScreenInfo( &w, &h, &d, &b, &p, &r, driver );
	*width = (int)w;
	*height = (int)h;
	*depth = (int)d;
	*bpp = (int)b;
	*pitch = (int)p;
	*refresh = (int)r;
}

FBCALL void fb_GfxScreenInfo64
	(
		long long *width,
		long long *height,
		long long *depth,
		long long *bpp,
		long long *pitch,
		long long *refresh,
		FBSTRING_ACCESS *driver
	)
{
	ssize_t w, h, d, b, p, r;
	fb_GfxScreenInfo( &w, &h, &d, &b, &p, &r, driver );
	*width = (long long)w;
	*height = (long long)h;
	*depth = (long long)d;
	*bpp = (long long)b;
	*pitch = (long long)p;
	*refresh = (long long)r;
}

/* end of gfx_screeninfo.c */
