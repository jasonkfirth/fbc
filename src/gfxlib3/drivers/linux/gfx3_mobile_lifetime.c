/*
    Project: FreeBASIC gfxlib3 Linux graphics
    ------------------------------------------

    File: linux/gfx3_mobile_lifetime.c

    Purpose:

        Keep the active mode alive while Linux mobile controls use its input.

    Responsibilities:

        - publish a mode only after it has been initialized
        - wait for an in-flight control before replacing or closing a mode

    This file intentionally does NOT contain:

        - keyboard or compositor protocol handling
        - mode creation, drawing, or application layout
*/

#include "gfx3_mobile.h"

#include <pthread.h>

/*
    The ordinary graphics lock is optional in non-MT runtime builds, but the
    Linux input method still has its own worker thread. This mutex therefore
    exists for the life of the process, independently of any mode or input
    mutex. A transition unpublishes the old mode before tearing it down and
    publishes the replacement only when initialization has finished.

    When both locks are needed, callers take FB_GRAPHICS_LOCK first. Keyboard
    controls use only the lease while contacting the session bus; they must
    not acquire the graphics lock until after releasing the lease.
*/
static pthread_mutex_t mobile_mode_mutex = PTHREAD_MUTEX_INITIALIZER;
static FB_GFX3_MODE *mobile_mode;

void fb_gfx3_linux_mode_transition_begin(void)
{
	pthread_mutex_lock(&mobile_mode_mutex);
	mobile_mode = NULL;
}

void fb_gfx3_linux_mode_transition_end(FB_GFX3_MODE *mode)
{
	mobile_mode = mode;
	pthread_mutex_unlock(&mobile_mode_mutex);
}

FB_GFX3_MODE *fb_gfx3_linux_mode_acquire(void)
{
	pthread_mutex_lock(&mobile_mode_mutex);
	return mobile_mode;
}

void fb_gfx3_linux_mode_release(void)
{
	pthread_mutex_unlock(&mobile_mode_mutex);
}

/* end of gfx3_mobile_lifetime.c */
