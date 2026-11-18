/*
 * FreeBASIC gfxlib2: gfx_screenlist.c
 * Purpose: Enumerate modes exposed by the selected native driver.
 * Responsibilities: Collect, sort and iterate a snapshot for each caller thread.
 * This file intentionally does NOT contain drawing algorithms or native presentation.
 */

/* Function to get supported video modes. */

#include "../fb_gfx.h"
#include <stdint.h>

typedef struct {
	int *modes;
	size_t count;
	size_t next;
} FB_SCREENLISTCTX;

static void fb_SCREENLISTCTX_Destructor(void *data)
{
	FB_SCREENLISTCTX *ctx = (FB_SCREENLISTCTX *)data;
	free(ctx->modes);
}

static void add_mode(FB_SCREENLISTCTX *ctx, int mode)
{
	size_t i;
	int *modes;

	for (i = 0; i < ctx->count; i++) {
		if (ctx->modes[i] == mode)
			return;
	}
	if (ctx->count >= SIZE_MAX / sizeof(*modes))
		return;
	modes = (int *)realloc(ctx->modes, (ctx->count + 1) * sizeof(*modes));
	if (!modes)
		return;
	ctx->modes = modes;
	ctx->modes[ctx->count++] = mode;
}

static int mode_sorter(const void *e1, const void *e2)
{
	int m1 = *(const int *)e1;
	int m2 = *(const int *)e2;

	if ((m1 >> 16) > (m2 >> 16))
		return 1;
	else if (((m1 >> 16) == (m2 >> 16)) && ((m1 & 0xFFFF) > (m2 & 0xFFFF)))
		return 1;
	else if (m1 == m2)
		return 0;
	else
		return -1;
}

FBCALL int fb_GfxScreenList(int depth)
{
	FB_SCREENLISTCTX *ctx;
	int result = 0;

	if (depth > 0)
		ctx = FB_TLSGETCTX(SCREENLIST);
	else
		ctx = (FB_SCREENLISTCTX *)fb_TlsGetCtxExisting(FB_TLSKEY_SCREENLIST);
	if (!ctx)
		return 0;

	if (depth > 0) {
		size_t i;
		free(ctx->modes);
		ctx->modes = NULL;
		ctx->count = 0;
		ctx->next = 0;

		/* Drivers share native display state. Once copied, the snapshot
		   belongs to this thread and survives graphics mode changes. */
		FB_GRAPHICS_LOCK();
		for (i = 0; fb_gfx_drivers_list[i]; i++) {
			const GFXDRIVER *driver = fb_gfx_drivers_list[i];
			if (driver->fetch_modes) {
				int count = 0;
				int *temp = driver->fetch_modes(depth, &count);
				if (temp) {
					int j;
					for (j = 0; j < count; j++)
						add_mode(ctx, temp[j]);
					free(temp);
				}
			}
		}
		FB_GRAPHICS_UNLOCK();
		if (ctx->count > 1)
			qsort(ctx->modes, ctx->count, sizeof(*ctx->modes), mode_sorter);
	}

	if (ctx->next < ctx->count)
		result = ctx->modes[ctx->next++];
	/* Completed walks release storage immediately. The TLS destructor
	   also handles a thread that exits before reading its last mode. */
	if (ctx->next == ctx->count)
		fb_TlsDelCtx(FB_TLSKEY_SCREENLIST);
	return result;
}

/* end of gfx_screenlist.c */
