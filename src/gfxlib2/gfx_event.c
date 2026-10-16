/*
    FreeBASIC graphics library
    File: gfx_event.c
    Purpose: Retrieve or peek at queued graphics events.
    Responsibilities: Poll foreground drivers and read the protected queue.
    This file intentionally does NOT contain native event translation.
*/

#include "fb_gfx.h"

FBCALL int fb_GfxEvent(EVENT *event)
{
	EVENT *e = NULL;

	FB_GRAPHICS_LOCK( );

	if (!__fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return FB_FALSE;
	}

	/* Foreground drivers fill this queue only when explicitly polled.
	   Do this before EVENT_LOCK because posting also takes the queue lock. */
	if (__fb_gfx->driver->poll_events)
		__fb_gfx->driver->poll_events();
	fb_hApplyPendingResize();

	EVENT_LOCK();
	if (__fb_gfx->event_head != __fb_gfx->event_tail) {
		e = &__fb_gfx->event_queue[__fb_gfx->event_head];
		if (event)
			__fb_gfx->event_head = (__fb_gfx->event_head + 1) & (MAX_EVENTS - 1);
	}
	if (e && event)
		fb_hMemCpy(event, e, sizeof(EVENT));
	EVENT_UNLOCK();

	FB_GRAPHICS_UNLOCK( );

	return e ? FB_TRUE : FB_FALSE;
}

/* end of gfx_event.c */
