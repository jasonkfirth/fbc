/*
	Project: FreeBASIC gfxlib2
	--------------------------

	File: gfx_touch.c

	Purpose:

		Expose a small touch-query API to FreeBASIC programs.

	Responsibilities:

		* report active touch contacts in logical gfxlib coordinates
		* provide rectangular and circular hit tests for touch UI code
		* offer a mouse-button fallback on platforms without native touch

	This file intentionally does NOT contain:

		* gesture recognition
		* platform event decoding
		* pressure, tilt, or swipe state
*/

#include "../fb_gfx.h"

#include <limits.h>

static void poll_events(void)
{
	if ((__fb_gfx) && (__fb_gfx->driver->poll_events))
		__fb_gfx->driver->poll_events();
}

static int get_mouse_touch_locked(int *x, int *y, int *id)
{
	int z = 0;
	int buttons = 0;
	int clip = 0;
	int fast_state;

	if (!__fb_gfx)
		return -1;

	/* The mouse fallback shares a cached sample when the driver can provide it.
	   A known outside-window pointer is not a touch contact and needs no query. */
	if (__fb_gfx->get_mouse_fast) {
		fast_state = __fb_gfx->get_mouse_fast(x, y, &z, &buttons, &clip);
		if (fast_state < 0)
			return -1;
		if ((fast_state == 0) && ((!__fb_gfx->driver) || (!__fb_gfx->driver->get_mouse) ||
			(__fb_gfx->driver->get_mouse(x, y, &z, &buttons, &clip) != 0)))
			return -1;
	} else {
		if ((!__fb_gfx->driver) || (!__fb_gfx->driver->get_mouse))
			return -1;
		if (__fb_gfx->driver->get_mouse(x, y, &z, &buttons, &clip) != 0)
			return -1;
	}

	if ((buttons & BUTTON_LEFT) == 0)
		return -1;

	if (id)
		*id = 0;

	return 0;
}

/*
	Use the driver-maintained pointer sample before taking its display lock.
	The X11 event thread holds that lock while presenting frames, so a cached
	mouse contact must not wait for presentation to finish.

	Return 1 for a contact, 0 for a known absence, and -1 when the driver needs
	its normal locked mouse query to initialize the cache.
*/
static int get_mouse_touch_cached(int *x, int *y, int *id)
{
	int z = 0;
	int buttons = 0;
	int clip = 0;
	int fast_state;

	if ((!__fb_gfx) || (!__fb_gfx->get_mouse_fast))
		return -1;

	fast_state = __fb_gfx->get_mouse_fast(x, y, &z, &buttons, &clip);
	if (fast_state == 0)
		return -1;
	if ((fast_state < 0) || ((buttons & BUTTON_LEFT) == 0))
		return 0;

	if (id)
		*id = 0;

	return 1;
}

static int get_platform_touch_locked(int index, int *x, int *y, int *id)
{
	if ((!__fb_gfx) || (!__fb_gfx->driver->get_touch))
		return -1;

	return __fb_gfx->driver->get_touch(index, x, y, id);
}

static int get_platform_touch_count_locked(void)
{
	if ((!__fb_gfx) || (!__fb_gfx->driver->get_touch_count))
		return 0;

	return __fb_gfx->driver->get_touch_count();
}

FBCALL ssize_t fb_GfxGetTouchCount(void)
{
	int count = 0;

	FB_GRAPHICS_LOCK( );

	poll_events( );

	if (__fb_gfx)
	{
		if (__fb_gfx->driver && __fb_gfx->driver->get_touch_count) {
			DRIVER_LOCK();
			count = get_platform_touch_count_locked();
			DRIVER_UNLOCK();
		}
		if (count <= 0)
		{
			int x = 0;
			int y = 0;
			int id = 0;
			int cached_contact;

			cached_contact = get_mouse_touch_cached(&x, &y, &id);
			if (cached_contact > 0) {
				count = 1;
			} else if (cached_contact < 0) {
				DRIVER_LOCK();
				if (get_mouse_touch_locked(&x, &y, &id) == 0)
					count = 1;
				DRIVER_UNLOCK();
			}
		}
	}

	FB_GRAPHICS_UNLOCK( );

	fb_ErrorSetNum(FB_RTERROR_OK);
	return count;
}

FBCALL ssize_t fb_GfxGetTouch(ssize_t index, ssize_t *x, ssize_t *y, ssize_t *id)
{
	int ix = -1;
	int iy = -1;
	int iid = 0;
	int failure = TRUE;
	int platform_count = 0;
	int use_platform_touch = FALSE;
	int cached_contact;

	FB_GRAPHICS_LOCK( );

	poll_events( );

	if (__fb_gfx && (index >= 0) && (index <= INT_MAX))
	{
		if (__fb_gfx->driver && __fb_gfx->driver->get_touch_count) {
			DRIVER_LOCK();
			platform_count = get_platform_touch_count_locked();
			if (platform_count > 0) {
				failure = get_platform_touch_locked((int)index, &ix, &iy, &iid);
				use_platform_touch = TRUE;
			}
			DRIVER_UNLOCK();
		}

		if ((!use_platform_touch) && (index == 0)) {
			cached_contact = get_mouse_touch_cached(&ix, &iy, &iid);
			if (cached_contact > 0) {
				failure = FALSE;
			} else if (cached_contact < 0) {
				DRIVER_LOCK();
				failure = get_mouse_touch_locked(&ix, &iy, &iid);
				DRIVER_UNLOCK();
			}
		}

		if (!failure && (__fb_gfx->scanline_size != 1))
			iy /= __fb_gfx->scanline_size;
	}

	FB_GRAPHICS_UNLOCK( );

	if (failure)
	{
		if (x)
			*x = -1;
		if (y)
			*y = -1;
		if (id)
			*id = -1;
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	if (x)
		*x = ix;
	if (y)
		*y = iy;
	if (id)
		*id = iid;

	return fb_ErrorSetNum(FB_RTERROR_OK);
}

FBCALL ssize_t fb_GfxGetTouchHit(ssize_t x1, ssize_t y1, ssize_t x2, ssize_t y2)
{
	ssize_t count;
	ssize_t i;

	if (x1 > x2)
	{
		ssize_t tmp = x1;
		x1 = x2;
		x2 = tmp;
	}
	if (y1 > y2)
	{
		ssize_t tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	count = fb_GfxGetTouchCount();
	for (i = 0; i < count; ++i)
	{
		ssize_t x = -1;
		ssize_t y = -1;
		ssize_t id = -1;

		if (fb_GfxGetTouch(i, &x, &y, &id) == 0)
		{
			if ((x >= x1) && (x <= x2) && (y >= y1) && (y <= y2))
			{
				fb_ErrorSetNum(FB_RTERROR_OK);
				return TRUE;
			}
		}
	}

	fb_ErrorSetNum(FB_RTERROR_OK);
	return FALSE;
}

FBCALL ssize_t fb_GfxGetTouchHitCircle(ssize_t x, ssize_t y, ssize_t radius)
{
	ssize_t count;
	ssize_t i;
	long long radius_ll = (long long)radius;
	long long radius_sq;

	if (radius_ll < 0)
	{
		if (radius_ll == LLONG_MIN)
			radius_ll = 3037000499LL;
		else
			radius_ll = -radius_ll;
	}

	if (radius_ll > 3037000499LL)
		radius_ll = 3037000499LL;

	radius_sq = radius_ll * radius_ll;

	count = fb_GfxGetTouchCount();
	for (i = 0; i < count; ++i)
	{
		ssize_t tx = -1;
		ssize_t ty = -1;
		ssize_t id = -1;

		if (fb_GfxGetTouch(i, &tx, &ty, &id) == 0)
		{
			long long dx = (long long)tx - (long long)x;
			long long dy = (long long)ty - (long long)y;

			if (dx > 3037000499LL)
				dx = 3037000499LL;
			else if (dx < -3037000499LL)
				dx = -3037000499LL;

			if (dy > 3037000499LL)
				dy = 3037000499LL;
			else if (dy < -3037000499LL)
				dy = -3037000499LL;

			if ((dx * dx) + (dy * dy) <= radius_sq)
			{
				fb_ErrorSetNum(FB_RTERROR_OK);
				return TRUE;
			}
		}
	}

	fb_ErrorSetNum(FB_RTERROR_OK);
	return FALSE;
}

/* end of gfx_touch.c */
