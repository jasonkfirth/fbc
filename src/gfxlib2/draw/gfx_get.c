/*
 * FreeBASIC gfxlib2: gfx_get.c
 * Purpose: Copy a clipped target region into a BASIC image buffer.
 * Responsibilities: target preparation, clipping and drawing semantics.
 * This file intentionally does NOT contain native windows or event dispatch.
 */

/* GET statement */

#include "../fb_gfx.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

static int gfx_get(void *target, float fx1, float fy1, float fx2, float fy2, unsigned char *dest, int coord_type, FBARRAY_ACCESS *array, int usenewheader )
{
	FB_GFXCTX *context;
	PUT_HEADER *header;
	int x1, y1, x2, y2, w, h, pitch;
	int64_t wide_w, wide_h, view_right, view_bottom;
	size_t row_bytes, pitch_bytes, pixel_bytes, header_bytes, destination_offset;
	uintptr_t array_start, destination_start;

	FB_GRAPHICS_LOCK( );

	if (!fb_gfx) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	context = fb_hGetContext( );
	fb_hPrepareTarget(context, target);
	fb_hSetPixelTransfer(context, MASK_A_32);

	fb_hFixRelative(context, coord_type, &fx1, &fy1, &fx2, &fy2);

	fb_hTranslateCoord(context, fx1, fy1, &x1, &y1);
	fb_hTranslateCoord(context, fx2, fy2, &x2, &y2);

	fb_hFixCoordsOrder(&x1, &y1, &x2, &y2);

	view_right = (int64_t)context->view_x + (int64_t)context->view_w;
	view_bottom = (int64_t)context->view_y + (int64_t)context->view_h;
	if ((context->view_w <= 0) || (context->view_h <= 0) ||
	    (x1 < context->view_x) || (y1 < context->view_y) ||
	    ((int64_t)x2 >= view_right) || ((int64_t)y2 >= view_bottom)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	wide_w = (int64_t)x2 - (int64_t)x1 + 1;
	wide_h = (int64_t)y2 - (int64_t)y1 + 1;
	if ((wide_w <= 0) || (wide_h <= 0) || (context->target_bpp <= 0)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	if (((uint64_t)wide_w > (uint64_t)INT_MAX) ||
	    ((uint64_t)wide_h > (uint64_t)INT_MAX)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	w = (int)wide_w;
	h = (int)wide_h;
	if ((size_t)w > ((size_t)-1 / (size_t)context->target_bpp)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	row_bytes = (size_t)w * (size_t)context->target_bpp;
	header_bytes = usenewheader ? sizeof( PUT_HEADER ) : sizeof( ((PUT_HEADER *)0)->old );
	if (usenewheader) {
		if (row_bytes > ((size_t)-1 - 0xFu)) {
			FB_GRAPHICS_UNLOCK( );
			return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		}
		pitch_bytes = (row_bytes + 0xFu) & ~(size_t)0xFu;
	} else {
		pitch_bytes = row_bytes;
	}
	if ((pitch_bytes > (size_t)INT_MAX) ||
	    ((size_t)h > ((size_t)-1 / pitch_bytes))) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}
	pitch = (int)pitch_bytes;
	pixel_bytes = pitch_bytes * (size_t)h;

	if ((array != NULL) && (array->size > 0)) {
		array_start = (uintptr_t)array->data;
		destination_start = (uintptr_t)dest;
		if (destination_start < array_start) {
			FB_GRAPHICS_UNLOCK( );
			return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		}
		destination_offset = (size_t)(destination_start - array_start);
		if ((destination_offset > array->size) ||
		    (header_bytes > (array->size - destination_offset)) ||
		    (pixel_bytes > (array->size - destination_offset - header_bytes))) {
			FB_GRAPHICS_UNLOCK( );
			return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
		}
	}

	header = (PUT_HEADER *)dest;
	if (!usenewheader) {
		/* use old-style header for compatibility */
		header->old.bpp = (unsigned short)((unsigned int)context->target_bpp & 0x7u);
		header->old.width = (unsigned short)((unsigned int)w & 0x1FFFu);
		header->old.height = (unsigned short)h;
		dest += header_bytes;
	} else {
		/* use new-style header */
		header->type = PUT_HEADER_NEW;
		header->width = (unsigned int)w;
		header->height = (unsigned int)h;
		header->bpp = context->target_bpp;
		header->pitch = (unsigned int)pitch_bytes;
		dest += header_bytes;
	}

	DRIVER_LOCK();

	for (;;) {
		fb_hPixelCpy(dest, context->line[y1] + ((ptrdiff_t)x1 * (ptrdiff_t)context->target_bpp), (size_t)w);
		dest += pitch;
		if (y1 == y2)
			break;
		++y1;
	}

	DRIVER_UNLOCK();

	FB_GRAPHICS_UNLOCK( );
	return fb_ErrorSetNum( FB_RTERROR_OK );
}

FBCALL int fb_GfxGet(void *target, float fx1, float fy1, float fx2, float fy2, unsigned char *dest, int coord_type, FBARRAY_ACCESS *array)
{
	return gfx_get( target, fx1, fy1, fx2, fy2, dest, coord_type, array, TRUE );
}

FBCALL int fb_GfxGetQB(void *target, float fx1, float fy1, float fx2, float fy2, unsigned char *dest, int coord_type, FBARRAY_ACCESS *array)
{
	return gfx_get( target, fx1, fy1, fx2, fy2, dest, coord_type, array, FALSE );
}

/* end of gfx_get.c */
