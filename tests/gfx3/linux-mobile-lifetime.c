/*
    Project: FreeBASIC gfxlib3 Linux tests
    -------------------------------------

    File: linux-mobile-lifetime.c

    Purpose:

        Check that mobile controls cannot use a closed graphics mode.

    Responsibilities:

        - hold a keyboard request while another thread closes the mode
        - verify a reopened mode receives only its own subsequent request
        - check that a session request does not hold the graphics lock

    This file intentionally does NOT contain:

        - a compositor, keyboard service, or physical input
        - rendering or real mode allocation

    Build and run on Linux:

        cc -std=gnu11 -Wall -Wextra -Werror -pthread -DENABLE_MT \
          -ffunction-sections -fdata-sections tests/gfx3/linux-mobile-lifetime.c \
          src/gfxlib3/api/gfx3_query_api.c \
          src/gfxlib3/drivers/linux/gfx3_mobile_lifetime.c \
          -Wl,--gc-sections -ldl -lm -o /tmp/gfx3-linux-mobile-lifetime
        /tmp/gfx3-linux-mobile-lifetime
*/

#include "../../src/gfxlib3/gfx3_api_internal.h"
#include "../../src/gfxlib3/gfx3_backend_null.h"
#include "../../src/gfxlib3/drivers/linux/gfx3_mobile.h"
#include "../../src/gfxlib2/drivers/mobile_control.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct TEST_STATE {
	pthread_mutex_t mutex;
	pthread_cond_t changed;
	int control_entered;
	int release_control;
	int close_started;
	int close_finished;
	int calls;
	FB_GFX3_INPUT_STATE *last_input;
} TEST_STATE;

static TEST_STATE test_state = {
	PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0, 0, 0, NULL
};
static pthread_mutex_t graphics_mutex = PTHREAD_MUTEX_INITIALIZER;
static FB_GFX3_MODE old_mode, new_mode;
static const FB_GFX3_BACKEND_VTABLE display_backend = { 0 };
const FB_GFX3_BACKEND_VTABLE __fb_gfx3_backend_null = { 0 };

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "gfx3-linux-mobile-lifetime: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

FBCALL void fb_GraphicsLock(void)
{
	pthread_mutex_lock(&graphics_mutex);
}

FBCALL void fb_GraphicsUnlock(void)
{
	pthread_mutex_unlock(&graphics_mutex);
}

void fb_gfx3_linux_mobile_control(int what, ssize_t *p1, ssize_t *p2,
	ssize_t *p3, ssize_t *p4, FB_GFX3_INPUT_STATE *input)
{
	(void)what;
	(void)p1;
	(void)p2;
	(void)p3;
	(void)p4;
	/* A slow session request must not hold the optional graphics lock. */
	require(pthread_mutex_trylock(&graphics_mutex) == 0,
		"the session request retained the graphics lock");
	pthread_mutex_unlock(&graphics_mutex);
	pthread_mutex_lock(&test_state.mutex);
	test_state.calls++;
	test_state.last_input = input;
	test_state.control_entered = 1;
	pthread_cond_broadcast(&test_state.changed);
	while (!test_state.release_control)
		pthread_cond_wait(&test_state.changed, &test_state.mutex);
	pthread_mutex_unlock(&test_state.mutex);
}

static void *request_keyboard(void *unused)
{
	ssize_t show = 1, p2 = 0, p3 = 0, p4 = 0;
	(void)unused;
	fb_gfx3_linux_control_mode(SET_SOFT_KEYBOARD, &show, &p2, &p3, &p4);
	return NULL;
}

static void *close_mode(void *unused)
{
	(void)unused;
	FB_GRAPHICS_LOCK();
	pthread_mutex_lock(&test_state.mutex);
	test_state.close_started = 1;
	pthread_cond_broadcast(&test_state.changed);
	pthread_mutex_unlock(&test_state.mutex);
	fb_gfx3_linux_mode_transition_begin();
	fb_gfx3_linux_mode_transition_end(NULL);
	FB_GRAPHICS_UNLOCK();
	pthread_mutex_lock(&test_state.mutex);
	test_state.close_finished = 1;
	pthread_cond_broadcast(&test_state.changed);
	pthread_mutex_unlock(&test_state.mutex);
	return NULL;
}

int main(void)
{
	pthread_t requester, closer;
	struct timespec deadline;
	ssize_t p1 = -2, p2 = -2, p3 = -2, p4 = -2;
	int result = 0;

	memset(&old_mode, 0, sizeof(old_mode));
	memset(&new_mode, 0, sizeof(new_mode));
	old_mode.context.renderer.backend_vtable = &display_backend;
	new_mode.context.renderer.backend_vtable = &display_backend;
	fb_gfx3_linux_mode_transition_begin();
	fb_gfx3_linux_mode_transition_end(&old_mode);
	require(clock_gettime(CLOCK_REALTIME, &deadline) == 0,
		"could not read the clock");
	deadline.tv_sec += 5;
	require(pthread_create(&requester, NULL, request_keyboard, NULL) == 0,
		"could not start the control request");
	pthread_mutex_lock(&test_state.mutex);
	while (!test_state.control_entered && result == 0)
		result = pthread_cond_timedwait(&test_state.changed, &test_state.mutex, &deadline);
	pthread_mutex_unlock(&test_state.mutex);
	require(result == 0, "the control request did not start");
	require(pthread_create(&closer, NULL, close_mode, NULL) == 0,
		"could not start mode shutdown");
	pthread_mutex_lock(&test_state.mutex);
	while (!test_state.close_started && result == 0)
		result = pthread_cond_timedwait(&test_state.changed, &test_state.mutex, &deadline);
	require(result == 0 && !test_state.close_finished,
		"mode shutdown did not wait for the control request");
	test_state.release_control = 1;
	pthread_cond_broadcast(&test_state.changed);
	pthread_mutex_unlock(&test_state.mutex);
	require(pthread_join(requester, NULL) == 0 && pthread_join(closer, NULL) == 0,
		"could not join the test threads");
	require(test_state.calls == 1 && test_state.last_input == &old_mode.input &&
		test_state.close_finished, "the first mode's request was lost");
	fb_gfx3_linux_control_mode(GET_SOFT_KEYBOARD, &p1, &p2, &p3, &p4);
	require(p1 == 0 && p2 == 0 && p3 == 0 && p4 == 0 && test_state.calls == 1,
		"a closed mode accepted a control request");
	fb_gfx3_linux_mode_transition_begin();
	fb_gfx3_linux_mode_transition_end(&new_mode);
	fb_gfx3_linux_control_mode(SET_SOFT_KEYBOARD_REGION, &p1, &p2, &p3, &p4);
	require(test_state.calls == 2 && test_state.last_input == &new_mode.input,
		"the reopened mode did not receive its own request");
	fb_gfx3_linux_mode_transition_begin();
	new_mode.context.renderer.backend_vtable = &__fb_gfx3_backend_null;
	fb_gfx3_linux_mode_transition_end(&new_mode);
	p1 = p2 = p3 = p4 = -2;
	fb_gfx3_linux_control_mode(GET_SOFT_KEYBOARD, &p1, &p2, &p3, &p4);
	require(p1 == 0 && p2 == 0 && p3 == 0 && p4 == 0 && test_state.calls == 2,
		"a headless mode accepted a keyboard request");
	fb_gfx3_linux_mode_transition_begin();
	fb_gfx3_linux_mode_transition_end(NULL);
	puts("GFX3_LINUX_MOBILE_LIFETIME_PASS");
	return EXIT_SUCCESS;
}

/* end of linux-mobile-lifetime.c */
