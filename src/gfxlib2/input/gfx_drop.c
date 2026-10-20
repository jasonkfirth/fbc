/*
 * FreeBASIC gfxlib2: gfx_drop.c
 * Purpose: Queue and retrieve copied native drop payloads.
 * Responsibilities: input state, event delivery and runtime input hooks.
 * This file intentionally does NOT contain pixel rasterization or mode allocation.
 */

/*
 * FreeBASIC gfxlib2: gfx_drop.c
 * Purpose: Expose copied drop payloads without changing SCREENEVENT ownership.
 * Responsibilities: Mode checks, event locking and BASIC-owned string copies.
 * Native window protocols and file access belong to other modules.
 */
#include "../fb_gfx.h"

FBCALL int fb_GfxDropEnable(int enabled)
{
	int result = FB_DROP_UNAVAILABLE;
	if (enabled != 0 && enabled != 1 && enabled != -1) return FB_DROP_INVALID;
	enabled = enabled != 0;
	FB_GRAPHICS_LOCK();
	if (__fb_gfx != NULL) {
		EVENT_LOCK();
		if (__fb_gfx->drops.supported) {
			__fb_gfx->drops.enabled = enabled;
			if (!enabled) fb_hDropClear(&__fb_gfx->drops);
			result = FB_DROP_OK;
		}
		EVENT_UNLOCK();
	}
	FB_GRAPHICS_UNLOCK();
	return result;
}

FBCALL int fb_GfxDropGet(int identifier, int index, FBSTRING *output)
{
	char *text = NULL;
	int result = FB_DROP_UNAVAILABLE;
	if (output == NULL) return FB_DROP_INVALID;
	FB_GRAPHICS_LOCK();
	if (__fb_gfx != NULL) {
		EVENT_LOCK();
		result = fb_hDropCopy(&__fb_gfx->drops, identifier, index, &text);
		EVENT_UNLOCK();
	}
	FB_GRAPHICS_UNLOCK();
	/* String allocation takes the runtime lock. Release input/graphics locks
	 * before assignment so platform producers never invert their lock order.
	 */
	if (result == FB_DROP_OK) {
		fb_StrAssign(output, -1, text, 0, FALSE);
		if ((size_t)output->len != strlen(text)) result = FB_DROP_FAILED;
		free(text);
	}
	return result;
}

int fb_hGfxDropSubmit(int kind, const char *data, size_t bytes, int count)
{
	EVENT event;
	int result;
	if (__fb_gfx == NULL) return FB_DROP_UNAVAILABLE;
	EVENT_LOCK();
	result = fb_hDropPublish(&__fb_gfx->drops, kind, data, bytes, count, &event);
	EVENT_UNLOCK();
	if (result == FB_DROP_OK) fb_hPostEvent(&event);
	return result;
}

/* end of gfx_drop.c */
