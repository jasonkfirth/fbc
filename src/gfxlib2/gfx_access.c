/* low level screen access routines */

#include "fb_gfx.h"

#ifdef HOST_JS
#define JS_UNLOCKS_PER_YIELD 4096
static int js_unlocks_before_yield = JS_UNLOCKS_PER_YIELD;
#endif

FBCALL void fb_GfxLock(void)
{
	FB_GRAPHICS_LOCK( );

	if (!__fb_gfx)
		return;

	if (__fb_gfx->lock_count == 0)
		__fb_gfx->driver->lock();

	++__fb_gfx->lock_count;
}

FBCALL void fb_GfxUnlock(int start_line, int end_line)
{
	FB_GFXCTX *context = fb_hGetContext();
#ifdef HOST_JS
	int do_yield = FALSE;
#endif

	if (!__fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return;
	}

	if (start_line < 0)
		start_line = 0;
	if (end_line < 0)
		end_line = __fb_gfx->h - 1;
	if ((__fb_gfx->visible_page == context->work_page) && (start_line <= end_line) && (end_line < __fb_gfx->h))
		fb_hMemSet(__fb_gfx->dirty + start_line, TRUE, end_line - start_line + 1);

	if (__fb_gfx->lock_count != 0) {
		--__fb_gfx->lock_count;
		if (__fb_gfx->lock_count == 0)
		{
			__fb_gfx->driver->unlock();
#ifdef HOST_JS
			do_yield = TRUE;
#endif
		}
	}

	FB_GRAPHICS_UNLOCK( );

#ifdef HOST_JS
	/*
	 * Browser builds are cooperative: long runs of software rendering can
	 * otherwise block input, timers, and canvas presentation until the whole
	 * frame finishes.  Yield occasionally at completed graphics unlocks, but
	 * not on every primitive, to keep old immediate-mode drawing code usable.
	 */
	if( do_yield )
	{
		js_unlocks_before_yield--;
		if( js_unlocks_before_yield <= 0 )
		{
			js_unlocks_before_yield = JS_UNLOCKS_PER_YIELD;
			fb_Delay( 0 );
		}
	}
#endif
}

FBCALL void *fb_GfxScreenPtr(void)
{
	FB_GFXCTX *context;
	void *p;

	FB_GRAPHICS_LOCK( );

	if (!__fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return NULL;
	}

	context = fb_hGetContext( );
	fb_hPrepareTarget(context, NULL);
	fb_hSetPixelTransfer(context, MASK_A_32);
	p = context->line[0];

	FB_GRAPHICS_UNLOCK( );
	return p;
}
