/*
 * FreeBASIC gfxlib2: gfx_control.c
 * Purpose: Implement SCREENCONTROL queries and state changes.
 * Responsibilities: driver selection, pages and mode queries.
 * This file intentionally does NOT contain drawing algorithms or native presentation.
 */

/* Screen states getter/setter routines. */

#include "../fb_gfx.h"
#include "../fb_gfx_gl.h"
#include "../drivers/mobile_control.h"
#ifdef HOST_WIN32
#include "../drivers/win32/gfx_metrics.h"
#endif

FBCALL void fb_GfxControl_s( int what, FBSTRING_ACCESS *param )
{
	FBSTRING_ACCESS *src;

	if (!param)
		return;

	FB_GRAPHICS_LOCK( );

	switch ( what ) {
	case GET_WINDOW_TITLE:
		if (!fb_window_title )
			src = &fb_ctx.null_desc;
		else
			src = fb_StrAllocTempDescF( fb_window_title, strlen(fb_window_title) + 1 );
		fb_StrAssign( param, -1, src, -1, FB_FALSE );
		break;

	case GET_DRIVER_NAME:
		if (!fb_gfx)
			src = &fb_ctx.null_desc;
		else
			src = fb_StrAllocTempDescF( fb_gfx->driver->name, strlen(fb_gfx->driver->name) + 1 );
		fb_StrAssign( param, -1, src, -1, FB_FALSE );
		break;

	case GET_GL_EXTENSIONS:
		src = &fb_ctx.null_desc;
#ifndef DISABLE_OPENGL
		if ((fb_gfx) && (fb_gfx->flags & OPENGL_SUPPORT))
			src = fb_StrAllocTempDescF( fb_gl_extensions, strlen(fb_gl_extensions) + 1 );
#endif
		fb_StrAssign( param, -1, src, -1, FB_FALSE );
		break;

	case SET_WINDOW_TITLE:
		fb_GfxSetWindowTitle(param);
		break;

	case SET_DRIVER_NAME:
		if (fb_gfx_driver_name)
			free(fb_gfx_driver_name);
		fb_gfx_driver_name = NULL;
		if (strlen(param->data) > 0)
			fb_gfx_driver_name = strdup(param->data);
		break;
	}

	FB_GRAPHICS_UNLOCK( );
}

FBCALL void fb_GfxControl_i( int what, ssize_t *param1, ssize_t *param2, ssize_t *param3, ssize_t *param4 )
{
	FB_GFXCTX *context;
	int res = 0;
	ssize_t res1 = 0, res2 = 0, res3 = 0, res4 = 0;
	ssize_t temp1 = (ssize_t)0x80000000;
	ssize_t temp2 = (ssize_t)0x80000000;
	ssize_t temp3 = (ssize_t)0x80000000;
	ssize_t temp4 = (ssize_t)0x80000000;

	if (!param1) param1 = &temp1;
	if (!param2) param2 = &temp2;
	if (!param3) param3 = &temp3;
	if (!param4) param4 = &temp4;

	if ((what == GET_WINDOW_METRICS) || (what == GET_SOFT_KEYBOARD) ||
	    (what == GET_TOUCH_CAPABILITY) || (what == GET_DEVICE_CLASS) ||
	    (what == SET_SOFT_KEYBOARD) || (what == SET_SOFT_KEYBOARD_REGION) ||
	    (what == CLEAR_SOFT_KEYBOARD_REGIONS)) {
		if (what == GET_DEVICE_CLASS) {
			*param1 = FB_GFX_DEVICE_CLASS_UNKNOWN;
			*param2 = *param3 = *param4 = 0;
		}
		if (what != GET_WINDOW_METRICS && what != GET_TOUCH_CAPABILITY &&
		    what != GET_DEVICE_CLASS) {
			int has_displayed_mode;

			/* A null mode has no focused surface and must not summon a session OSK. */
			FB_GRAPHICS_LOCK();
			has_displayed_mode = (fb_gfx != NULL) &&
				(fb_gfx->driver != NULL) &&
				(fb_gfx->driver != &fb_gfxDriverNull);
			FB_GRAPHICS_UNLOCK();
			if (!has_displayed_mode) {
				if (what == GET_SOFT_KEYBOARD)
					*param1 = *param2 = *param3 = *param4 = 0;
				return;
			}
		}
#if defined(HOST_JS) || defined(HOST_ANDROID) || defined(HOST_LINUX)
		fb_hMobileControl(what, param1, param2, param3, param4);
#else
		if (what == GET_WINDOW_METRICS) {
			FB_GRAPHICS_LOCK();
			fb_hApplyPendingResize();
			if (fb_gfx) {
				*param1 = fb_gfx->w;
				*param2 = fb_gfx->h;
			} else {
				fb_hScreenInfo(param1, param2, &res3, &res4);
			}
			*param3 = 96;
#ifdef HOST_WIN32
			*param3 = fb_hWindowDpi(fb_gfx ? fb_hGetWindowHandle() : 0);
#endif
			*param4 = 0;
			FB_GRAPHICS_UNLOCK();
		} else if (what == GET_SOFT_KEYBOARD) {
			*param1 = *param2 = *param3 = *param4 = 0;
		} else if (what == GET_TOUCH_CAPABILITY) {
			*param1 = -2;
			*param2 = *param3 = *param4 = 0;
		} else if (what == GET_DEVICE_CLASS) {
			*param2 = *param3 = *param4 = 0;
		}
#endif
		return;
	}

	FB_GRAPHICS_LOCK( );

	context = fb_hGetContext();

	switch ( what ) {
	case GET_WINDOW_POS:
		if ((fb_gfx) && (fb_gfx->driver->set_window_pos))
			res = fb_gfx->driver->set_window_pos(0x80000000, 0x80000000);
		res1 = (short)(res & 0xFFFF);
		res2 = res >> 16;
		break;

	case GET_WINDOW_HANDLE:
		if (fb_gfx) {
			res1 = fb_hGetWindowHandle();
			res2 = fb_hGetDisplayHandle();
		}
		break;

	case GET_DESKTOP_SIZE:
		{
			ssize_t w = 0, h = 0, depth = 0, refresh = 0;
			fb_hScreenInfo( &w, &h, &depth, &refresh );
			res1 = w;
			res2 = h;
		}
		break;

	case GET_SCREEN_SIZE:
		if (fb_gfx) {
			res1 = fb_gfx->w;
			res2 = fb_gfx->h;
		}
		break;

	case GET_SCREEN_DEPTH:
		if (fb_gfx)
			res1 = fb_gfx->depth;
		break;

	case GET_SCREEN_BPP:
		if (fb_gfx)
			res1 = fb_gfx->bpp;
		break;

	case GET_SCREEN_PITCH:
		if (fb_gfx)
			res1 = fb_gfx->pitch;
		break;

	case GET_SCREEN_REFRESH:
		if (fb_gfx)
			res1 = fb_gfx->refresh_rate;
		break;

	case GET_TRANSPARENT_COLOR:
		if ((fb_gfx) && (fb_gfx->bpp > 1))
			res1 = MASK_COLOR_32;
		break;

	case GET_VIEWPORT:
		if (fb_gfx) {
			res1 = context->view_x;
			res2 = context->view_y;
			res3 = context->view_x + context->view_w - 1;
			res4 = context->view_y + context->view_h - 1;
		}
		break;

	case GET_PEN_POS:
		if (fb_gfx) {
			res1 = context->last_x;
			res2 = context->last_y;
		}
		break;

	case GET_COLOR:
		if (fb_gfx) {
			res1 = context->fg_color;
			res2 = context->bg_color;
		}
		break;

	case GET_ALPHA_PRIMITIVES:
		if (fb_gfx)
			res1 = (fb_gfx->flags & ALPHA_PRIMITIVES) ? FB_TRUE : FB_FALSE;
		break;

	case GET_HIGH_PRIORITY:
		if (fb_gfx)
			res1 = (fb_gfx->flags & GFX_HIGH_PRIORITY) ? FB_TRUE : FB_FALSE;
		break;

	case GET_SCANLINE_SIZE:
		if (fb_gfx)
			res1 = fb_gfx->scanline_size;
		break;

	case GET_X86_MMX_ENABLED:
		if (fb_gfx)
			res1 = (fb_gfx->flags & X86_MMX_ENABLED) ? FB_TRUE : FB_FALSE;
		break;

	case SET_WINDOW_POS:
		if ((fb_gfx) && (fb_gfx->driver->set_window_pos))
			fb_gfx->driver->set_window_pos(*param1, *param2);
		break;

	case SET_PEN_POS:
		if (fb_gfx) {
			if (*param1 != (ssize_t)0x80000000)
				context->last_x = *param1;
			if (*param2 != (ssize_t)0x80000000)
				context->last_y = *param2;
		}
		break;

	case SET_ALPHA_PRIMITIVES:
		if ((fb_gfx) && (*param1 != (ssize_t)0x80000000)) {
			if (*param1)
				fb_gfx->flags |= ALPHA_PRIMITIVES;
			else
				fb_gfx->flags &= ~ALPHA_PRIMITIVES;
			/* Pixel transfer routines depend on the current alpha mode. */
			fb_hSetupFuncs(fb_gfx->bpp);
		}
		break;

#ifndef DISABLE_OPENGL
	case GET_GL_COLOR_BITS:
		res1 = fb_gl_params.color_bits;
		break;

	case GET_GL_COLOR_RED_BITS:
		res1 = fb_gl_params.color_red_bits;
		break;

	case GET_GL_COLOR_GREEN_BITS:
		res1 = fb_gl_params.color_green_bits;
		break;

	case GET_GL_COLOR_BLUE_BITS:
		res1 = fb_gl_params.color_blue_bits;
		break;

	case GET_GL_COLOR_ALPHA_BITS:
		res1 = fb_gl_params.color_alpha_bits;
		break;

	case GET_GL_DEPTH_BITS:
		res1 = fb_gl_params.depth_bits;
		break;

	case GET_GL_STENCIL_BITS:
		res1 = fb_gl_params.stencil_bits;
		break;

	case GET_GL_ACCUM_BITS:
		res1 = fb_gl_params.accum_bits;
		break;

	case GET_GL_ACCUM_RED_BITS:
		res1 = fb_gl_params.accum_red_bits;
		break;

	case GET_GL_ACCUM_GREEN_BITS:
		res1 = fb_gl_params.accum_green_bits;
		break;

	case GET_GL_ACCUM_BLUE_BITS:
		res1 = fb_gl_params.accum_blue_bits;
		break;

	case GET_GL_ACCUM_ALPHA_BITS:
		res1 = fb_gl_params.accum_alpha_bits;
		break;

	case GET_GL_NUM_SAMPLES:
		res1 = fb_gl_params.num_samples;
		break;

	case GET_GL_2D_MODE:
		/* return the active setting (not the initializing setting) */
		res1 = fb_gl_params.mode_2d;
		break;

	case GET_GL_SCALE:
		/* return the active setting (not the initializing setting) */
		res1 = fb_gl_params.scale;
		break;

	case SET_GL_COLOR_BITS:
		fb_gl_params.color_bits = *param1;
		break;

	case SET_GL_COLOR_RED_BITS:
		fb_gl_params.color_red_bits = *param1;
		break;

	case SET_GL_COLOR_GREEN_BITS:
		fb_gl_params.color_green_bits = *param1;
		break;

	case SET_GL_COLOR_BLUE_BITS:
		fb_gl_params.color_blue_bits = *param1;
		break;

	case SET_GL_COLOR_ALPHA_BITS:
		fb_gl_params.color_alpha_bits = *param1;
		break;

	case SET_GL_DEPTH_BITS:
		fb_gl_params.depth_bits = *param1;
		break;

	case SET_GL_STENCIL_BITS:
		fb_gl_params.stencil_bits = *param1;
		break;

	case SET_GL_ACCUM_BITS:
		fb_gl_params.accum_bits = *param1;
		break;

	case SET_GL_ACCUM_RED_BITS:
		fb_gl_params.accum_red_bits = *param1;
		break;

	case SET_GL_ACCUM_GREEN_BITS:
		fb_gl_params.accum_green_bits = *param1;
		break;

	case SET_GL_ACCUM_BLUE_BITS:
		fb_gl_params.accum_blue_bits = *param1;
		break;

	case SET_GL_ACCUM_ALPHA_BITS:
		fb_gl_params.accum_alpha_bits = *param1;
		break;

	case SET_GL_NUM_SAMPLES:
		fb_gl_params.num_samples = *param1;
		break;

	case SET_X86_MMX_ENABLED:
#ifdef HOST_X86
		if (fb_gfx) {
			if (fb_CpuDetect() & 0x800000) {
				if (*param1) {
					fb_gfx->flags |= X86_MMX_ENABLED;
				} else {
					fb_gfx->flags &= ~X86_MMX_ENABLED;
				}
			} else {
				fb_gfx->flags &= ~X86_MMX_ENABLED;
			}
		}
#endif
		context->last_target = NULL;
		break;

	case SET_GL_2D_MODE:
		/* set the initial 2d mode only; we don't want to
		   change the active mode in use.  The activie mode
		   only gets set in driver_init() */
		fb_gl_params.init_mode_2d = *param1;
		break;

	case SET_GL_SCALE:
		/* set the initial scale only; we don't want to
		   change the active scale in use.  The activie scale
		   only gets set in driver_init() */
		fb_gl_params.init_scale = *param1;
		break;
#endif

	case POLL_EVENTS:
		if ((fb_gfx) && (fb_gfx->driver->poll_events))
			fb_gfx->driver->poll_events();
		break;

	}

	FB_GRAPHICS_UNLOCK( );

	if (what < SET_FIRST_SETTER) {
		*param1 = res1;
		*param2 = res2;
		*param3 = res3;
		*param4 = res4;
	}
}

FBCALL void fb_GfxControl_i32( int what, int *param1, int *param2, int *param3, int *param4 )
{
	ssize_t p1 = (ssize_t)0x80000000;
	ssize_t p2 = (ssize_t)0x80000000;
	ssize_t p3 = (ssize_t)0x80000000;
	ssize_t p4 = (ssize_t)0x80000000;
	if( param1 ) p1 = (ssize_t)*param1;
	if( param2 ) p2 = (ssize_t)*param2;
	if( param3 ) p3 = (ssize_t)*param3;
	if( param4 ) p4 = (ssize_t)*param4;
	fb_GfxControl_i( what, &p1, &p2, &p3, &p4 );
	if( param1 ) *param1 = (int)p1;
	if( param2 ) *param2 = (int)p2;
	if( param3 ) *param3 = (int)p3;
	if( param4 ) *param4 = (int)p4;
}

FBCALL void fb_GfxControl_i64( int what, long long *param1, long long *param2, long long *param3, long long *param4 )
{
	ssize_t p1 = (ssize_t)0x80000000;
	ssize_t p2 = (ssize_t)0x80000000;
	ssize_t p3 = (ssize_t)0x80000000;
	ssize_t p4 = (ssize_t)0x80000000;
	if( param1 ) p1 = (ssize_t)*param1;
	if( param2 ) p2 = (ssize_t)*param2;
	if( param3 ) p3 = (ssize_t)*param3;
	if( param4 ) p4 = (ssize_t)*param4;
	fb_GfxControl_i( what, &p1, &p2, &p3, &p4 );
	if( param1 ) *param1 = (long long)p1;
	if( param2 ) *param2 = (long long)p2;
	if( param3 ) *param3 = (long long)p3;
	if( param4 ) *param4 = (long long)p4;
}

/* end of gfx_control.c */
