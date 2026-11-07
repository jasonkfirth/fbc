/*
 * Project: FreeBASIC Linux graphics tests
 * --------------------------------------
 *
 * File: linux-maliit-lifecycle.c
 * Purpose: Check that a captured input callback cannot outlive its mode.
 * Responsibilities: Race a text commit against cancellation and reopening.
 * This file intentionally does NOT connect to a desktop or input method.
 *
 * Build and run on Linux with GIO development headers:
 *   cc -std=c11 -Wall -Wextra -Werror -pthread \
 *      -ffunction-sections -fdata-sections tests/gfx/linux-maliit-lifecycle.c \
 *      -Wl,--gc-sections $(pkg-config --cflags --libs gio-2.0) -ldl \
 *      -o /tmp/linux-maliit-lifecycle
 *   /tmp/linux-maliit-lifecycle
 */
#define _POSIX_C_SOURCE 200809L
#include "../../src/gfxlib2/drivers/linux/fb_linux_maliit.c"
#include <time.h>

typedef struct TEST_STATE {
	pthread_mutex_t signal;
	pthread_cond_t changed;
	int entered;
	int release_callback;
	int cancel_started;
	int cancel_complete;
} TEST_STATE;

typedef struct TEST_CLIENT {
	TEST_STATE *state;
	int accepted;
} TEST_CLIENT;

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "linux-maliit-lifecycle: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void receive_key(void *user_data, uint32_t key,
	FB_LINUX_MALIIT_INPUT_KIND kind)
{
	TEST_CLIENT *client = user_data;
	TEST_STATE *state = client->state;
	(void)key;
	(void)kind;
	pthread_mutex_lock(&state->signal);
	state->entered++;
	pthread_cond_broadcast(&state->changed);
	while (!state->release_callback)
		pthread_cond_wait(&state->changed, &state->signal);
	client->accepted++;
	pthread_mutex_unlock(&state->signal);
}

static void *post_text(void *unused)
{
	(void)unused;
	fb_hLinuxMaliitPostUtf8("ab", 2);
	return NULL;
}

static void *cancel_callback(void *user_data)
{
	TEST_STATE *state = user_data;
	pthread_mutex_lock(&state->signal);
	state->cancel_started = 1;
	pthread_cond_broadcast(&state->changed);
	pthread_mutex_unlock(&state->signal);
	fb_hLinuxMaliitCancelPending();
	pthread_mutex_lock(&state->signal);
	state->cancel_complete = 1;
	pthread_cond_broadcast(&state->changed);
	pthread_mutex_unlock(&state->signal);
	return NULL;
}

int main(void)
{
	TEST_STATE state = {
		PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0, 0
	};
	TEST_CLIENT old_client = { &state, 0 };
	TEST_CLIENT new_client = { &state, 0 };
	struct timespec deadline;
	pthread_t worker, canceller;
	int result = 0;

	fb_hLinuxMaliitSetKeyCallback(receive_key, &old_client);
	require(pthread_create(&worker, NULL, post_text, NULL) == 0,
		"could not start the callback worker");
	require(clock_gettime(CLOCK_REALTIME, &deadline) == 0,
		"could not read the clock");
	deadline.tv_sec += 5;
	pthread_mutex_lock(&state.signal);
	while (state.entered == 0 && result == 0)
		result = pthread_cond_timedwait(&state.changed, &state.signal, &deadline);
	pthread_mutex_unlock(&state.signal);
	require(result == 0, "the callback did not start");

	/* A repeated registration must not wait for an in-flight callback. */
	fb_hLinuxMaliitSetKeyCallback(receive_key, &old_client);

	/* Shutdown must wait for the captured text commit before freeing its mode. */
	require(pthread_create(&canceller, NULL, cancel_callback, &state) == 0,
		"could not start the shutdown worker");
	pthread_mutex_lock(&state.signal);
	while (!state.cancel_started && result == 0)
		result = pthread_cond_timedwait(&state.changed, &state.signal, &deadline);
	require(result == 0 && !state.cancel_complete,
		"shutdown returned while the callback was active");
	state.release_callback = 1;
	pthread_cond_broadcast(&state.changed);
	pthread_mutex_unlock(&state.signal);
	require(pthread_join(worker, NULL) == 0, "could not join the worker");
	require(pthread_join(canceller, NULL) == 0, "could not join shutdown");
	require(old_client.accepted == 2 && state.cancel_complete,
		"shutdown did not wait for the complete text commit");
	fb_hLinuxMaliitPostUtf8("d", 1);
	require(old_client.accepted == 2, "a closed mode received text");
	fb_hLinuxMaliitSetKeyCallback(receive_key, &new_client);
	fb_hLinuxMaliitPostUtf8("c", 1);
	require(new_client.accepted == 1, "the reopened mode did not receive text");
	fb_hLinuxMaliitCancelPending();
	fb_hLinuxMaliitPostUtf8("d", 1);
	require(new_client.accepted == 1, "a closed mode received text");
	pthread_cond_destroy(&state.changed);
	pthread_mutex_destroy(&state.signal);
	puts("LINUX_MALIIT_LIFECYCLE_PASS");
	return EXIT_SUCCESS;
}

/* end of linux-maliit-lifecycle.c */
