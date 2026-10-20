/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/time_timer.c
 * Purpose: timer() function.
 * Responsibilities: Target-specific runtime support for js.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <emscripten.h>

FBCALL double fb_Timer( void )
{
	/*
		FreeBASIC programs commonly store TIMER in SINGLE variables and
		compare it against small frame intervals.  Returning Unix epoch seconds
		loses those small intervals once the value is rounded to SINGLE.

		Emscripten's monotonic clock is relative to the page/runtime lifetime,
		which keeps the value small and preserves sub-frame precision.
	*/
	return emscripten_get_now() * 0.001;
}

/* end of drivers/js/time_timer.c */
