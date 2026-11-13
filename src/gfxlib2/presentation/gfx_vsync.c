/*
 * FreeBASIC gfxlib2: gfx_vsync.c
 * Purpose: Wait for vertical synchronization through the active driver.
 * Responsibilities: presentation helpers and driver synchronization.
 * This file intentionally does NOT contain BASIC coordinate interpretation or input dispatch.
 */

/* vertical sync routine */

#include "../fb_gfx.h"

FBCALL int fb_GfxWaitVSync(void)
{
	FB_GRAPHICS_LOCK( );

	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum( FB_RTERROR_ILLEGALFUNCTIONCALL );
	}

	if (fb_gfx->driver->wait_vsync)
		fb_gfx->driver->wait_vsync();

	FB_GRAPHICS_UNLOCK( );
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

/* end of gfx_vsync.c */
