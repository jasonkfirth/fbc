/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_file_api.h

    Purpose:

        Expose private file-codec views and the bitmap decoder used by the
        GPU asset loader.

    Responsibilities:

        - decode one BLOAD-compatible bitmap into owned staging pixels
        - share checked CPU-image and screen-page views with file codecs
        - report the exact staging layout selected for a GPU surface

    This file intentionally does NOT contain:

        - public FreeBASIC declarations
        - GPU allocation or upload commands
        - image format implementation details
*/

#ifndef __FB_GFX3_FILE_API_H__
#define __FB_GFX3_FILE_API_H__

#include "fb_gfx3.h"

typedef struct FB_GFX3_FILE_VIEW {
	unsigned char *pixels;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t bytes_per_pixel;
	unsigned char *allocation;
	struct FB_GFX3_DRAW_STATE *state;
} FB_GFX3_FILE_VIEW;

int fb_gfx3_file_prepare_view_locked(void *image, int read_screen,
	FB_GFX3_FILE_VIEW *view);
void fb_gfx3_file_release_view(FB_GFX3_FILE_VIEW *view);
int fb_gfx3_file_commit_screen_locked(FB_GFX3_FILE_VIEW *view,
	uint32_t width, uint32_t height);

int fb_gfx3_file_load_bitmap_pixels_locked(FBSTRING *filename,
	uint32_t depth, unsigned char **pixels, uint32_t *width,
	uint32_t *height, uint32_t *pitch);

#endif

/* end of gfx3_file_api.h */
