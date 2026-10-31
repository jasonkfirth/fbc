/*
 * Project: FreeBASIC gfxlib3 Android
 * File: gfx3_android_viewport.h
 * Purpose: Share the usable presentation rectangle with the GLES backend.
 * Responsibilities: Bound a matching screen page to the Java-observed area.
 * This file does not own input, EGL surfaces or application layout.
 */
#ifndef FB_GFX3_ANDROID_VIEWPORT_H
#define FB_GFX3_ANDROID_VIEWPORT_H
#include <stdint.h>
void fb_gfx3_android_usable_viewport(uint32_t source_width, uint32_t source_height,
	uint32_t *width, uint32_t *height);
#endif
/* end of gfx3_android_viewport.h */
