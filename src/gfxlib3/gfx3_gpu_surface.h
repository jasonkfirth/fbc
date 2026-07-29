/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_gpu_surface.h

    Purpose:

        Define the private bridge between opaque public GPU surfaces and
        ordinary gfxlib3 drawing entry points.

    Responsibilities:

        - validate opaque GPU surface pointers without dereferencing strangers
		- destroy live extension surfaces before their owning mode shuts down
		- route standard pixel, line, box, and ellipse targets to GPU commands

    This file intentionally does NOT contain:

        - the public FreeBASIC declarations from fbgfx3.bi
		- CPU FB.IMAGE parsing or storage
		- backend-specific GPU objects
*/

#ifndef __FB_GFX3_GPU_SURFACE_H__
#define __FB_GFX3_GPU_SURFACE_H__

#include "gfx3_compat.h"

int fb_gfx3_gpu_surface_lookup_locked(void *target, FB_GFX3_MODE *mode,
	FB_GFX3_SURFACE **surface);
void fb_gfx3_gpu_surfaces_destroy_all_locked(FB_GFX3_MODE *mode);

int fb_gfx3_gpu_surface_pset(FB_GFX3_SURFACE *surface,
	FB_GFX3_DRAW_STATE *state, float x, float y, uint32_t color,
	uint32_t flags, int preset);
int fb_gfx3_gpu_surface_point(FB_GFX3_SURFACE *surface, float x, float y,
	uint32_t *color);
int fb_gfx3_gpu_surface_line(FB_GFX3_SURFACE *surface,
	FB_GFX3_DRAW_STATE *state, float x1, float y1, float x2, float y2,
	uint32_t color, int type, uint32_t style, uint32_t flags);
int fb_gfx3_gpu_surface_ellipse(FB_GFX3_SURFACE *surface,
	FB_GFX3_DRAW_STATE *state, float x, float y, float radius,
	uint32_t color, float aspect, float start, float end, int filled,
	uint32_t flags);

#endif

/* end of gfx3_gpu_surface.h */
