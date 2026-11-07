/*
 * FreeBASIC Linux graphics tests
 * ------------------------------
 *
 * File: linux-maliit-queue-fallback.c
 *
 * Purpose:
 *
 *     Check asynchronous Maliit failure and keyboard fallback ordering.
 *
 * Responsibilities:
 *
 *     - hold a fallback show while requests and cancellation continue
 *     - verify that cancellation follows an in-flight show with a hide
 *     - check recovery when the request worker cannot be started
 *
 * This file intentionally does NOT contact the user's session bus or a
 * desktop keyboard. GTestDBus supplies a private bus without Maliit.
 *
 * Build and run from the repository root:
 *
 *     cc -std=c11 -Wall -Wextra -Werror -pthread \
 *         tests/gfx/linux-maliit-queue-fallback.c \
 *         -o linux-maliit-queue-fallback $(pkg-config --cflags --libs gio-2.0) -ldl
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <gio/gio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int fail_worker_creation;
static int test_desktop_session;
static int test_without_bus;

static char *test_getenv(const char *name)
{
	static char lomiri[] = "Lomiri";
	static char desktop[] = "GNOME";

	if (strcmp(name, "XDG_CURRENT_DESKTOP") == 0)
		return test_desktop_session ? desktop : lomiri;
	if (test_without_bus && strcmp(name, "DBUS_SESSION_BUS_ADDRESS") == 0)
		return NULL;
	return getenv(name);
}

static int test_pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
	void *(*start_routine)(void *), void *argument)
{
	if (fail_worker_creation)
		return EAGAIN;
	return pthread_create(thread, attributes, start_routine, argument);
}

#define pthread_create test_pthread_create
#define getenv test_getenv
#include "../../src/gfxlib2/drivers/linux/fb_linux_maliit.c"
#undef getenv
#undef pthread_create

typedef struct TEST_FALLBACK {
	pthread_mutex_t mutex;
	pthread_cond_t changed;
	int shows;
	int hides;
	int release_show;
} TEST_FALLBACK;

static TEST_FALLBACK test_fallback = {
	PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0
};

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "linux-maliit-queue-fallback: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void fallback(int visible)
{
	pthread_mutex_lock(&test_fallback.mutex);
	if (visible) {
		test_fallback.shows++;
		pthread_cond_broadcast(&test_fallback.changed);
		while (!test_fallback.release_show)
			pthread_cond_wait(&test_fallback.changed, &test_fallback.mutex);
	} else {
		test_fallback.hides++;
		pthread_cond_broadcast(&test_fallback.changed);
	}
	pthread_mutex_unlock(&test_fallback.mutex);
}

static int wait_for_count(int *count, int target)
{
	struct timespec deadline;
	int result;

	if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
		return 0;
	deadline.tv_sec += 5;
	pthread_mutex_lock(&test_fallback.mutex);
	while (*count < target) {
		if (pthread_cond_timedwait(&test_fallback.changed,
		    &test_fallback.mutex, &deadline) != 0)
			break;
	}
	result = *count >= target;
	pthread_mutex_unlock(&test_fallback.mutex);
	return result;
}

static int wait_for_worker(void)
{
	const struct timespec delay = { 0, 10000000L };
	int attempt, running;

	for (attempt = 0; attempt < 300; attempt++) {
		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		running = fb_hLinuxMaliitState.request_worker_running;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
		if (!running)
			return 1;
		(void)nanosleep(&delay, NULL);
	}
	return 0;
}

int main(void)
{
	GTestDBus *bus;
	struct timespec started, finished;
	int64_t elapsed_ns;
	int index, running, pending;

	require(setenv("XDG_CURRENT_DESKTOP", "Lomiri", 1) == 0,
		"could not set the desktop");
	require(unsetenv("WAYLAND_DISPLAY") == 0, "could not clear Wayland");
	bus = g_test_dbus_new(G_TEST_DBUS_NONE);
	g_test_dbus_up(bus);

	/* Failure to create a worker must leave a subsequent request usable. */
	fail_worker_creation = 1;
	require(fb_hLinuxMaliitQueueKeyboard(1, fallback) == 0,
		"failed worker creation was reported as queued");
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	running = fb_hLinuxMaliitState.request_worker_running;
	pending = fb_hLinuxMaliitState.request_pending;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	require(!running && !pending, "failed worker creation left pending work");
	fail_worker_creation = 0;

	require(fb_hLinuxMaliitQueueKeyboard(1, fallback) == 1,
		"show was not queued after worker creation recovered");
	require(wait_for_count(&test_fallback.shows, 1),
		"missing Maliit service did not invoke the fallback");
	require(clock_gettime(CLOCK_MONOTONIC, &started) == 0, "clock failed");
	for (index = 0; index < 100; index++)
		require(fb_hLinuxMaliitQueueKeyboard(index & 1, fallback) == 1,
			"request was not coalesced while fallback was blocked");
	fb_hLinuxMaliitCancelPending();
	require(clock_gettime(CLOCK_MONOTONIC, &finished) == 0, "clock failed");
	elapsed_ns = (finished.tv_sec - started.tv_sec) * INT64_C(1000000000) +
		finished.tv_nsec - started.tv_nsec;
	require(elapsed_ns < INT64_C(500000000),
		"a blocked fallback delayed requests or mode cancellation");
	pthread_mutex_lock(&test_fallback.mutex);
	test_fallback.release_show = 1;
	pthread_cond_broadcast(&test_fallback.changed);
	pthread_mutex_unlock(&test_fallback.mutex);
	require(wait_for_count(&test_fallback.hides, 1),
		"cancelled fallback show was not followed by a hide");
	require(wait_for_worker(), "fallback worker did not finish");
	pthread_mutex_lock(&test_fallback.mutex);
	require(test_fallback.shows == 1 && test_fallback.hides == 1,
		"cancelled queued requests reached the fallback");
	pthread_mutex_unlock(&test_fallback.mutex);

	/* A non-Lomiri session uses the same ordered worker for its fallback. */
	test_desktop_session = 1;
	pthread_mutex_lock(&test_fallback.mutex);
	test_fallback.release_show = 0;
	pthread_mutex_unlock(&test_fallback.mutex);
	require(fb_hLinuxMaliitQueueKeyboard(1, fallback) == 1,
		"desktop fallback show was not queued");
	require(wait_for_count(&test_fallback.shows, 2),
		"desktop fallback did not receive the show");
	require(fb_hLinuxMaliitQueueKeyboard(0, fallback) == 1,
		"desktop fallback hide was not queued");
	pthread_mutex_lock(&test_fallback.mutex);
	require(test_fallback.hides == 1,
		"desktop fallback hide overtook the in-flight show");
	test_fallback.release_show = 1;
	pthread_cond_broadcast(&test_fallback.changed);
	pthread_mutex_unlock(&test_fallback.mutex);
	require(wait_for_count(&test_fallback.hides, 2),
		"desktop fallback did not receive the hide");
	require(wait_for_worker(), "desktop hide worker did not finish");
	test_without_bus = 1;
	require(fb_hLinuxMaliitQueueKeyboard(1, fallback) == 0,
		"a desktop without a session bus started a worker");

	g_test_dbus_down(bus);
	g_object_unref(bus);
	puts("LINUX_MALIIT_QUEUE_FALLBACK_PASS");
	return EXIT_SUCCESS;
}

/* end of linux-maliit-queue-fallback.c */
