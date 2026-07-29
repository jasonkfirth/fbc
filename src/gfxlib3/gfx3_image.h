/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_image.h

    Purpose:

        Define the CPU-visible FB.IMAGE compatibility boundary used by
        gfxlib3.

    Responsibilities:

        - validate old QB and current FreeBASIC image headers
        - expose checked row geometry to drawing and transfer code
        - provide CPU primitive and PUT compatibility helpers

    This file intentionally does NOT contain:

        - public fb_Gfx* entry points
        - screen page or GPU resource ownership
        - file-format loading or saving
*/

#ifndef __FB_GFX3_IMAGE_H__
#define __FB_GFX3_IMAGE_H__

#include "gfx3_compat.h"
#include "../rtlib/fb_gfx_private.h"

enum {
	FB_GFX3_IMAGE_HEADER_NEW = 7,
	FB_GFX3_IMAGE_NEW_HEADER_SIZE = 32,
	/* Private marker stored only in the unused tail of gfxlib3-owned headers. */
	FB_GFX3_IMAGE_CACHE_METADATA_MAGIC = 0x33494D47u,
	FB_GFX3_IMAGE_CACHE_METADATA_EXTERNAL_WRITE = 0x00000001u
};

typedef struct FB_GFX3_IMAGE_VIEW {
	unsigned char *header;
	unsigned char *pixels;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t bytes_per_pixel;
	uint32_t header_size;
} FB_GFX3_IMAGE_VIEW;

int fb_gfx3_image_parse(void *image, FB_GFX3_IMAGE_VIEW *view);
int fb_gfx3_image_initialize_header(void *image, int use_new_header,
	uint32_t width, uint32_t height, uint32_t bytes_per_pixel,
	uint32_t pitch);
void fb_gfx3_image_cache_metadata_initialize(FB_GFX3_IMAGE_VIEW *view);
int fb_gfx3_image_cache_metadata_get(const FB_GFX3_IMAGE_VIEW *view,
	uint32_t *generation, int *external_write);
void fb_gfx3_image_cache_metadata_touch(FB_GFX3_IMAGE_VIEW *view);
void fb_gfx3_image_cache_metadata_mark_external(FB_GFX3_IMAGE_VIEW *view);
uint32_t fb_gfx3_image_fix_color(uint32_t bytes_per_pixel, uint32_t color);
uint32_t fb_gfx3_image_expand_color(uint32_t bytes_per_pixel,
	uint32_t color);
uint32_t fb_gfx3_image_get_pixel_raw(const FB_GFX3_IMAGE_VIEW *view,
	int x, int y);
void fb_gfx3_image_set_pixel_raw(FB_GFX3_IMAGE_VIEW *view, int x, int y,
	uint32_t color);
void fb_gfx3_image_set_primitive_pixel(FB_GFX3_IMAGE_VIEW *view,
	const FB_GFX3_DRAW_STATE *state, int x, int y, uint32_t color);

int fb_gfx3_image_pset(FB_GFX3_IMAGE_VIEW *view,
	FB_GFX3_DRAW_STATE *state, float x, float y, uint32_t color,
	uint32_t flags, int preset);
uint32_t fb_gfx3_image_point(const FB_GFX3_IMAGE_VIEW *view,
	float x, float y);
int fb_gfx3_image_line(FB_GFX3_IMAGE_VIEW *view,
	FB_GFX3_DRAW_STATE *state, float x1, float y1, float x2, float y2,
	uint32_t color, int type, uint32_t style, uint32_t flags);
int fb_gfx3_image_ellipse(FB_GFX3_IMAGE_VIEW *view,
	FB_GFX3_DRAW_STATE *state, float x, float y, float radius,
	uint32_t color, float aspect, float start, float end, int filled,
	uint32_t flags);

int fb_gfx3_image_put_pixels(unsigned char *source,
	unsigned char *destination, uint32_t width, uint32_t height,
	uint32_t source_pitch, uint32_t destination_pitch,
	uint32_t bytes_per_pixel, int mode, int alpha, BLENDER *blender,
	void *parameter);
/* The public PUT compatibility wrapper holds FB_GRAPHICS_LOCK for this call. */
int fb_gfx3_image_put_surface(FB_GFX3_SURFACE *destination_surface,
	const FB_GFX3_RECT *clip, const FB_GFX3_IMAGE_VIEW *source_view,
	int source_x, int source_y, uint32_t width, uint32_t height,
	int destination_x, int destination_y, int mode, int alpha,
	BLENDER *blender, void *parameter);

#endif

/* end of gfx3_image.h */
