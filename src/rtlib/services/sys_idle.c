/*
 * Project: FreeBASIC runtime: sys_idle.c
 * Purpose: Run a small periodic task without a busy loop or a hidden thread.
 * Responsibilities: Bounded waits, coalesced deadlines and callback reentry checks.
 * Network operations and application lifetime policy are owned by the caller.
 * File: services/sys_idle.c
 * This file intentionally does NOT contain: unrelated runtime subsystems.
 */
#include "fb_services.h"
#include <math.h>

/* One state belongs to one caller thread. A callback may stop its own state,
 * but may not recursively step or reinitialize it. No runtime lock is held
 * across a callback. Zero-initialize the state before its first use.
 */
#define FB_IDLE_SIGNATURE 0x46424944
#define FB_IDLE_MAX_INTERVAL 86400000u /* one day, in milliseconds */
#define FB_IDLE_MAX_WAIT 60000        /* retain a bounded cancellation point */

FBCALL int fb_IdleInit(FB_IDLE_STATE *state, uint32_t interval_ms)
{
	double now;
	if (state == NULL || interval_ms == 0 ||
	    interval_ms > FB_IDLE_MAX_INTERVAL)
		return FB_SERVICE_INVALID;
	if (state->reserved == FB_IDLE_SIGNATURE && state->busy)
		return FB_SERVICE_INVALID;
	if (!fb_hIdleNow(&now) || !isfinite(now))
		return FB_SERVICE_UNAVAILABLE;
	state->deadline = now + (double)interval_ms / 1000.0;
	state->previous = now;
	state->interval_ms = interval_ms;
	state->running = TRUE;
	state->busy = FALSE;
	state->reserved = FB_IDLE_SIGNATURE;
	return FB_SERVICE_OK;
}

FBCALL int fb_IdleStep(FB_IDLE_STATE *state, FB_IDLE_CALLBACK callback,
	void *user, int maximum_wait_ms)
{
	double now;
	double remaining;
	int wait_ms;
	if (state == NULL || callback == NULL ||
	    state->reserved != FB_IDLE_SIGNATURE || state->busy ||
	    state->interval_ms == 0 || state->interval_ms > FB_IDLE_MAX_INTERVAL ||
	    maximum_wait_ms < 0 || maximum_wait_ms > FB_IDLE_MAX_WAIT ||
	    !isfinite(state->deadline) || !isfinite(state->previous))
		return FB_SERVICE_INVALID;
	if (!state->running)
		return FB_SERVICE_OK;
	if (!fb_hIdleNow(&now) || !isfinite(now))
		return FB_SERVICE_UNAVAILABLE;
	/* Generic legacy targets can have only a wall clock. Rearm on a clock
	 * regression rather than spinning or waiting for an obsolete deadline.
	 */
	if (now < state->previous)
		state->deadline = now + (double)state->interval_ms / 1000.0;
	state->previous = now;
	remaining = (state->deadline - now) * 1000.0;
	if (remaining > 0 && maximum_wait_ms != 0) {
		wait_ms = maximum_wait_ms;
		if (remaining < (double)wait_ms)
			wait_ms = (int)ceil(remaining);
		/* SLEEP's keyboard wakeup can leave a pending key spinning an idle
		 * loop. Delay is independent of keyboard input and never sees zero.
		 */
		fb_Delay(wait_ms);
		if (!fb_hIdleNow(&now) || !isfinite(now))
			return FB_SERVICE_UNAVAILABLE;
		if (now < state->previous)
			state->deadline = now + (double)state->interval_ms / 1000.0;
	}
	if (now >= state->deadline) {
		state->busy = TRUE;
		if (!callback(user))
			state->running = FALSE;
		state->busy = FALSE;
		if (!fb_hIdleNow(&now) || !isfinite(now)) {
			state->running = FALSE;
			return FB_SERVICE_UNAVAILABLE;
		}
		/* Slow tasks coalesce missed ticks. Never flood a server with catch-up
		 * requests after suspend, a long callback or a clock correction.
		 */
		state->deadline = now + (double)state->interval_ms / 1000.0;
	}
	state->previous = now;
	return FB_SERVICE_OK;
}

FBCALL int fb_IdleStop(FB_IDLE_STATE *state)
{
	if (state == NULL || state->reserved != FB_IDLE_SIGNATURE)
		return FB_SERVICE_INVALID;
	state->running = FALSE;
	return FB_SERVICE_OK;
}

/* end of services/sys_idle.c */
