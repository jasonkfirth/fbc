/*
 * FreeBASIC graphics tests
 * ------------------------
 * File: linux-osk-fallback.c
 * Purpose: Check session keyboard visibility and Hildon/Maliit fallbacks.
 * Responsibilities: Exercise the shared OSK cache and service precedence.
 * This file intentionally does not start a desktop session or an input method.
 *
 * Build on Linux with: cc -std=c11 -Wall -Wextra -Werror -pthread
 *     tests/gfx/linux-osk-fallback.c -o linux-osk-fallback
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* A controlled clock keeps repeated queries inside the same cache interval. */
static time_t test_now = 100;
static int test_session_bus;
static int test_phosh_available;
static int test_phosh_visible;
static int test_phosh_queries;
static int test_hildon_im;
static int test_hildon_available;
static int test_hildon_queries;
static int test_query_available;
static int test_kwin_available;
static int test_kwin_visible;
static int test_kwin_queries;
static int test_maliit_available;
static int test_maliit_visible;
static int test_maliit_queries;
static int test_maliit_requests;
static int test_fail_thread;
static int test_thread_attempts;
static int test_maliit_queue_supported;
static char test_system_command[256];

static time_t test_time(time_t *result)
{
	if (result)
		*result = test_now;
	return test_now;
}

static char *test_getenv(const char *name)
{
	static char bus_address[] = "test-session-bus";
	static char hildon_im[] = "him";
	if (test_session_bus && strcmp(name, "DBUS_SESSION_BUS_ADDRESS") == 0)
		return bus_address;
	if (test_hildon_im && strcmp(name, "QT_IM_MODULE") == 0)
		return hildon_im;
	return NULL;
}

static FILE *test_popen(const char *command, const char *mode)
{
	FILE *stream;
	(void)mode;
	stream = tmpfile();
	if (!stream)
		return NULL;
	if (strstr(command, "GetNameOwner")) {
		test_hildon_queries++;
		test_query_available = test_hildon_available;
		if (test_hildon_available)
			fputs("(':1.42',)\n", stream);
	} else {
		test_phosh_queries++;
		test_query_available = test_phosh_available;
		if (test_phosh_available)
			fputs(test_phosh_visible ? "(<true>,)\n" : "(<false>,)\n", stream);
	}
	rewind(stream);
	return stream;
}

static int test_pclose(FILE *stream)
{
	(void)fclose(stream);
	/* The upper byte is the child exit status used by WEXITSTATUS. */
	return test_query_available ? 0 : 256;
}

static int test_system(const char *command)
{
	int length = snprintf(test_system_command, sizeof(test_system_command),
		"%s", command);
	return ((length < 0) || ((size_t)length >= sizeof(test_system_command))) ?
		-1 : 0;
}

static int test_pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
	void *(*start_routine)(void *), void *argument)
{
	test_thread_attempts++;
	if (test_fail_thread)
		return EAGAIN;
	return pthread_create(thread, attributes, start_routine, argument);
}

#define time test_time
#define getenv test_getenv
#define popen test_popen
#define pclose test_pclose
#define system test_system
#define pthread_create test_pthread_create
#include "../../src/gfxlib2/drivers/linux/fb_linux_osk.h"
#undef pthread_create
#undef system
#undef pclose
#undef popen
#undef getenv
#undef time

int fb_hLinuxMaliitQueryKeyboard(int *visible)
{
	test_maliit_queries++;
	*visible = test_maliit_visible;
	return test_maliit_available;
}

int fb_hLinuxKwinQueryKeyboard(int *visible)
{
	test_kwin_queries++;
	*visible = test_kwin_visible;
	return test_kwin_available;
}

int fb_hLinuxMaliitQueueKeyboard(int visible,
	FB_LINUX_MALIIT_FALLBACK_CALLBACK fallback)
{
	(void)visible;
	(void)fallback;
	test_maliit_requests++;
	return test_maliit_queue_supported;
}

static void expect_state(const char *name, int expected_available,
	int expected_visible)
{
	int visible = 123;
	int available = fb_hLinuxOskQuery(&visible);
	if (available != expected_available || visible != expected_visible) {
		fprintf(stderr, "%s: got availability %d and visibility %d\n",
			name, available, visible);
		exit(EXIT_FAILURE);
	}
}

static void wait_for_query(void)
{
	struct timespec pause_time = { 0, 10000000 };
	int attempt, running;
	for (attempt = 0; attempt < 100; attempt++) {
		pthread_mutex_lock(&fb_hLinuxOskMutex);
		running = fb_hLinuxOskQueryRunning;
		pthread_mutex_unlock(&fb_hLinuxOskMutex);
		if (!running)
			return;
		(void)nanosleep(&pause_time, NULL);
	}
	fprintf(stderr, "keyboard worker did not finish\n");
	exit(EXIT_FAILURE);
}

static void reset_state(void)
{
	wait_for_query();
	pthread_mutex_lock(&fb_hLinuxOskMutex);
	fb_hLinuxOskAvailable = -1;
	fb_hLinuxOskVisible = -2;
	fb_hLinuxOskCachedAt = 0;
	fb_hLinuxOskGeneration = 0;
	fb_hLinuxOskQueryGeneration = 0;
	pthread_mutex_unlock(&fb_hLinuxOskMutex);
	test_now = 100;
	test_session_bus = 0;
	test_phosh_available = 0;
	test_phosh_visible = 0;
	test_phosh_queries = 0;
	test_hildon_im = 0;
	test_hildon_available = 0;
	test_hildon_queries = 0;
	test_query_available = 0;
	test_kwin_available = 0;
	test_kwin_visible = -2;
	test_kwin_queries = 0;
	test_maliit_available = -1;
	test_maliit_visible = -1;
	test_maliit_queries = 0;
	test_maliit_requests = 0;
	test_fail_thread = 0;
	test_thread_attempts = 0;
	test_maliit_queue_supported = 1;
	test_system_command[0] = '\0';
}

int main(void)
{
	reset_state();
	expect_state("Maliit without session bus", -1, -1);
	test_maliit_visible = 0;
	expect_state("Maliit without session bus, repeated", -1, 0);
	if (test_maliit_queries != 2)
		return EXIT_FAILURE;

	reset_state();
	test_session_bus = 1;
	expect_state("Maliit after missing Phosh", -1, -1);
	wait_for_query();
	test_maliit_visible = 0;
	expect_state("Maliit after missing Phosh, repeated", -1, 0);
	if (test_phosh_queries != 1 || test_maliit_queries != 2)
		return EXIT_FAILURE;
	fb_hLinuxOskRequest(1);
	if (test_maliit_requests != 1)
		return EXIT_FAILURE;

	reset_state();
	test_session_bus = 1;
	test_phosh_available = 1;
	test_phosh_visible = 1;
	test_maliit_available = 0;
	test_maliit_visible = -2;
	expect_state("Phosh initial fallback", 0, -2);
	wait_for_query();
	expect_state("Phosh visible", -1, -1);
	test_phosh_visible = 0;
	expect_state("Phosh cached", -1, -1);
	test_now++;
	expect_state("Phosh refresh pending", -1, -1);
	wait_for_query();
	expect_state("Phosh cache refreshed", -1, 0);
	if (test_phosh_queries != 2 || test_maliit_queries != 1)
		return EXIT_FAILURE;

	reset_state();
	test_session_bus = 1;
	test_kwin_available = -1;
	test_kwin_visible = -1;
	test_maliit_available = 0;
	test_maliit_visible = -2;
	expect_state("KWin initial fallback", 0, -2);
	wait_for_query();
	expect_state("KWin visible", -1, -1);
	test_kwin_visible = 0;
	test_now++;
	expect_state("KWin refresh pending", -1, -1);
	wait_for_query();
	expect_state("KWin hidden", -1, 0);
	if (test_phosh_queries != 2 || test_kwin_queries != 2)
		return EXIT_FAILURE;

	reset_state();
	test_session_bus = 1;
	test_hildon_im = 1;
	test_hildon_available = 1;
	test_maliit_available = 0;
	test_maliit_visible = -2;
	test_maliit_queue_supported = 0;
	expect_state("Hildon initial query", 0, -2);
	wait_for_query();
	expect_state("Hildon keyboard available", -1, -2);
	if ((test_hildon_queries != 1) || (test_kwin_queries != 0))
		return EXIT_FAILURE;
	fb_hLinuxOskRequest(1);
	if (!strstr(test_system_command,
	    "org.maemo.him.set_visible true"))
		return EXIT_FAILURE;
	fb_hLinuxOskRequest(0);
	if (!strstr(test_system_command,
	    "org.maemo.him.set_visible false"))
		return EXIT_FAILURE;

	reset_state();
	test_session_bus = 1;
	test_fail_thread = 1;
	expect_state("thread failure fallback", -1, -1);
	expect_state("thread failure cached", -1, -1);
	if (test_thread_attempts != 1 || test_phosh_queries != 0)
		return EXIT_FAILURE;
	test_now++;
	expect_state("thread failure retry", -1, -1);
	if (test_thread_attempts != 2)
		return EXIT_FAILURE;

	puts("linux-osk-fallback: passed");
	return EXIT_SUCCESS;
}

/* end of linux-osk-fallback.c */
