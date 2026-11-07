/*
 * FreeBASIC Linux graphics tests
 * ------------------------------
 *
 * File: linux-osk-async.c
 *
 * Purpose:
 *
 *     Verify that a slow desktop keyboard query does not block graphics.
 *
 * Responsibilities:
 *
 *     - hold a simulated Phosh query until the caller has returned
 *     - verify that the completed result becomes available through the cache
 *
 * Run from the repository root:
 *
 *     cc -std=c11 -Wall -Wextra -Werror -pthread \
 *         tests/gfx/linux-osk-async.c -o linux-osk-async
 *
 * This file intentionally does NOT start a desktop session or input method.
 */

#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t test_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t test_changed = PTHREAD_COND_INITIALIZER;
static int test_query_started;
static int test_query_released;
static int test_caller_done;
static int test_query_count;
static char test_last_command[192];

static char *test_getenv(const char *name)
{
	static char bus_address[] = "test-session-bus";
	return strcmp(name, "DBUS_SESSION_BUS_ADDRESS") == 0 ? bus_address : NULL;
}

static FILE *test_popen(const char *command, const char *mode)
{
	FILE *stream;
	(void)command;
	(void)mode;
	pthread_mutex_lock(&test_mutex);
	test_query_started = 1;
	test_query_count++;
	pthread_cond_broadcast(&test_changed);
	while (!test_query_released)
		pthread_cond_wait(&test_changed, &test_mutex);
	pthread_mutex_unlock(&test_mutex);
	stream = tmpfile();
	if (stream) {
		fputs("(<true>,)\n", stream);
		rewind(stream);
	}
	return stream;
}

static int test_pclose(FILE *stream)
{
	(void)fclose(stream);
	return 0;
}

static int test_system(const char *command)
{
	(void)snprintf(test_last_command, sizeof(test_last_command), "%s", command);
	return 0;
}

#define getenv test_getenv
#define popen test_popen
#define pclose test_pclose
#define system test_system
#include "../../src/gfxlib2/drivers/linux/fb_linux_osk.h"
#undef system
#undef pclose
#undef popen
#undef getenv

int fb_hLinuxMaliitQueryKeyboard(int *visible)
{
	*visible = -2;
	return 0;
}

int fb_hLinuxKwinQueryKeyboard(int *visible)
{
	*visible = -2;
	return 0;
}

int fb_hLinuxMaliitQueueKeyboard(int visible,
	FB_LINUX_MALIIT_FALLBACK_CALLBACK fallback)
{
	(void)visible;
	(void)fallback;
	return 0;
}

static void *run_query(void *unused)
{
	int visible;
	(void)unused;
	(void)fb_hLinuxOskQuery(&visible);
	pthread_mutex_lock(&test_mutex);
	test_caller_done = 1;
	pthread_cond_broadcast(&test_changed);
	pthread_mutex_unlock(&test_mutex);
	return NULL;
}

int main(void)
{
	pthread_t caller;
	struct timespec deadline, pause_time = { 0, 10000000 };
	int completed_before_release, visible, available, attempt;

	if (pthread_create(&caller, NULL, run_query, NULL) != 0)
		return EXIT_FAILURE;
	if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
		return EXIT_FAILURE;
	deadline.tv_sec += 2;
	pthread_mutex_lock(&test_mutex);
	while (!test_query_started) {
		if (pthread_cond_timedwait(&test_changed, &test_mutex, &deadline) != 0)
			break;
	}
	while (test_query_started && !test_caller_done) {
		if (pthread_cond_timedwait(&test_changed, &test_mutex, &deadline) != 0)
			break;
	}
	completed_before_release = test_query_started && test_caller_done;
	pthread_mutex_unlock(&test_mutex);
	if (completed_before_release)
		fb_hLinuxOskRequest(1);
	pthread_mutex_lock(&test_mutex);
	test_query_released = 1;
	pthread_cond_broadcast(&test_changed);
	pthread_mutex_unlock(&test_mutex);
	pthread_join(caller, NULL);
	if (!completed_before_release) {
		fprintf(stderr, "keyboard query blocked the caller\n");
		return EXIT_FAILURE;
	}

	available = 0;
	visible = -2;
	for (attempt = 0; attempt < 100; attempt++) {
		available = fb_hLinuxOskQuery(&visible);
		if (available == -1 && visible == -1)
			break;
		(void)nanosleep(&pause_time, NULL);
	}
	if (available != -1 || visible != -1) {
		fprintf(stderr, "completed keyboard state was not cached\n");
		return EXIT_FAILURE;
	}
	pthread_mutex_lock(&test_mutex);
	attempt = test_query_count;
	pthread_mutex_unlock(&test_mutex);
	if (attempt != 2) {
		fprintf(stderr, "stale result survived a keyboard request\n");
		return EXIT_FAILURE;
	}
	if (!strstr(test_last_command, " >/dev/null 2>&1 &")) {
		fprintf(stderr, "direct Phosh request was not sent in the background\n");
		return EXIT_FAILURE;
	}
	fb_hLinuxOskRequestFallback(1);
	if (strstr(test_last_command, " >/dev/null 2>&1 &")) {
		fprintf(stderr, "worker fallback did not wait for its request\n");
		return EXIT_FAILURE;
	}
	puts("linux-osk-async: passed");
	return EXIT_SUCCESS;
}

/* end of linux-osk-async.c */
