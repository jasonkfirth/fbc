/*
 * FreeBASIC Linux graphics tests
 * ------------------------------
 *
 * File: linux-osk-phosh-peer.c
 *
 * Purpose:
 *
 *     Exercise the shared on-screen keyboard fallback against a private bus.
 *
 * Responsibilities:
 *
 *     - provide the Phosh visibility method and property on a test bus
 *     - provide KWin's visibility property on the same isolated bus
 *     - hold a show while the caller queues a later hide
 *     - verify the final observed state through the real query path
 *
 * This file intentionally does NOT connect to the user's session bus or
 * control an actual desktop keyboard.
 *
 * Build and run from the repository root:
 *
 *     cc -std=c11 -Wall -Wextra -Werror -pthread \
 *         tests/gfx/linux-osk-phosh-peer.c -o linux-osk-phosh-peer \
 *         $(pkg-config --cflags --libs gio-2.0) -ldl
 */

#define _POSIX_C_SOURCE 200809L

#include <gio/gio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../src/gfxlib2/drivers/linux/fb_linux_maliit.c"
#include "../../src/gfxlib2/drivers/linux/fb_linux_osk.h"

typedef struct TEST_OSK {
	pthread_mutex_t mutex;
	pthread_cond_t changed;
	int ready;
	int failed;
	int shows;
	int hides;
	int visible;
	int release_show;
	GMainLoop *loop;
} TEST_OSK;

static TEST_OSK test_osk = {
	PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0, 0, 0, 0, NULL
};

static const char osk_xml[] =
	"<node><interface name='sm.puri.OSK0'>"
	"<method name='SetVisible'><arg type='b' direction='in'/></method>"
	"<property name='Visible' type='b' access='read'/>"
	"</interface><interface name='org.kde.kwin.VirtualKeyboard'>"
	"<property name='visible' type='b' access='read'/>"
	"</interface></node>";

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "linux-osk-phosh-peer: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static void osk_call(GDBusConnection *connection, const char *sender,
	const char *object_path, const char *interface_name, const char *method,
	GVariant *parameters, GDBusMethodInvocation *invocation, void *user_data)
{
	TEST_OSK *state = user_data;
	gboolean visible;
	(void)connection;
	(void)sender;
	(void)object_path;
	(void)interface_name;
	(void)method;
	g_variant_get(parameters, "(b)", &visible);
	pthread_mutex_lock(&state->mutex);
	if (visible) {
		state->shows++;
		pthread_cond_broadcast(&state->changed);
		while (!state->release_show)
			pthread_cond_wait(&state->changed, &state->mutex);
	} else {
		state->hides++;
	}
	state->visible = visible != 0;
	pthread_cond_broadcast(&state->changed);
	pthread_mutex_unlock(&state->mutex);
	g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
}

static GVariant *osk_property(GDBusConnection *connection, const char *sender,
	const char *object_path, const char *interface_name, const char *name,
	GError **error, void *user_data)
{
	TEST_OSK *state = user_data;
	int visible;
	(void)connection;
	(void)sender;
	(void)object_path;
	(void)interface_name;
	(void)error;
	if (strcmp(name, "Visible") != 0 && strcmp(name, "visible") != 0)
		return NULL;
	pthread_mutex_lock(&state->mutex);
	visible = state->visible;
	pthread_mutex_unlock(&state->mutex);
	return g_variant_new_boolean(visible);
}

static void *run_osk(void *unused)
{
	static const GDBusInterfaceVTable vtable = { osk_call, osk_property, NULL, { 0 } };
	GMainContext *context = NULL;
	GMainLoop *loop = NULL;
	GDBusConnection *connection = NULL;
	GDBusNodeInfo *node = NULL;
	GVariant *reply = NULL;
	GError *error = NULL;
	guint registration = 0, kwin_registration = 0;
	guint32 owner = 0;
	(void)unused;

	context = g_main_context_new();
	g_main_context_push_thread_default(context);
	connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (!connection)
		goto finished;
	node = g_dbus_node_info_new_for_xml(osk_xml, &error);
	if (!node)
		goto finished;
	registration = g_dbus_connection_register_object(connection, "/sm/puri/OSK0",
		node->interfaces[0], &vtable, &test_osk, NULL, &error);
	if (!registration)
		goto finished;
	kwin_registration = g_dbus_connection_register_object(connection,
		"/VirtualKeyboard", node->interfaces[1], &vtable, &test_osk,
		NULL, &error);
	if (!kwin_registration)
		goto finished;
	reply = g_dbus_connection_call_sync(connection, "org.freedesktop.DBus",
		"/org/freedesktop/DBus", "org.freedesktop.DBus", "RequestName",
		g_variant_new("(su)", "sm.puri.OSK0", 4u), G_VARIANT_TYPE("(u)"),
		G_DBUS_CALL_FLAGS_NONE, 2000, NULL, &error);
	if (!reply)
		goto finished;
	g_variant_get(reply, "(u)", &owner);
	g_variant_unref(reply);
	reply = NULL;
	if (owner != 1)
		goto finished;
	reply = g_dbus_connection_call_sync(connection, "org.freedesktop.DBus",
		"/org/freedesktop/DBus", "org.freedesktop.DBus", "RequestName",
		g_variant_new("(su)", "org.kde.KWin", 4u), G_VARIANT_TYPE("(u)"),
		G_DBUS_CALL_FLAGS_NONE, 2000, NULL, &error);
	if (!reply)
		goto finished;
	g_variant_get(reply, "(u)", &owner);
	g_variant_unref(reply);
	reply = NULL;
	if (owner != 1)
		goto finished;
	loop = g_main_loop_new(context, FALSE);
	pthread_mutex_lock(&test_osk.mutex);
	test_osk.loop = loop;
	test_osk.ready = 1;
	pthread_cond_broadcast(&test_osk.changed);
	pthread_mutex_unlock(&test_osk.mutex);
	g_main_loop_run(loop);

finished:
	pthread_mutex_lock(&test_osk.mutex);
	if (!test_osk.ready)
		test_osk.failed = 1;
	pthread_cond_broadcast(&test_osk.changed);
	pthread_mutex_unlock(&test_osk.mutex);
	if (error)
		g_error_free(error);
	if (reply)
		g_variant_unref(reply);
	if (registration)
		g_dbus_connection_unregister_object(connection, registration);
	if (kwin_registration)
		g_dbus_connection_unregister_object(connection, kwin_registration);
	if (node)
		g_dbus_node_info_unref(node);
	if (connection)
		g_object_unref(connection);
	if (loop)
		g_main_loop_unref(loop);
	g_main_context_pop_thread_default(context);
	g_main_context_unref(context);
	return NULL;
}

static int wait_for_count(int *count, int target)
{
	struct timespec deadline;
	int result;

	if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
		return 0;
	deadline.tv_sec += 5;
	pthread_mutex_lock(&test_osk.mutex);
	while (*count < target && !test_osk.failed) {
		if (pthread_cond_timedwait(&test_osk.changed, &test_osk.mutex,
		    &deadline) != 0)
			break;
	}
	result = *count >= target;
	pthread_mutex_unlock(&test_osk.mutex);
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
	pthread_t server;
	struct timespec started, finished;
	const struct timespec delay = { 0, 10000000L };
	int64_t elapsed_ns;
	int result, available = 0, visible = -2, attempt;

	require(setenv("XDG_CURRENT_DESKTOP", "Phosh", 1) == 0, "setenv failed");
	require(unsetenv("MIR_SOCKET") == 0, "unsetenv failed");
	bus = g_test_dbus_new(G_TEST_DBUS_NONE);
	g_test_dbus_up(bus);
	visible = 123;
	require(fb_hLinuxKwinQueryKeyboard(&visible) == 0 && visible == -2,
		"an absent KWin service did not report unknown visibility");
	require(pthread_create(&server, NULL, run_osk, NULL) == 0,
		"could not start the mock service");
	require(wait_for_count(&test_osk.ready, 1), "mock service did not start");
	require(!fb_hLinuxKwinQueryKeyboard(NULL), "KWin query accepted a null result");
	visible = -2;
	require(fb_hLinuxKwinQueryKeyboard(&visible) == -1 && visible == 0,
		"KWin hidden state was not read");
	pthread_mutex_lock(&test_osk.mutex);
	test_osk.visible = 1;
	pthread_mutex_unlock(&test_osk.mutex);
	require(fb_hLinuxKwinQueryKeyboard(&visible) == -1 && visible == -1,
		"KWin visible state was not read");
	pthread_mutex_lock(&test_osk.mutex);
	test_osk.visible = 0;
	pthread_mutex_unlock(&test_osk.mutex);

	fb_hLinuxOskRequest(1);
	require(wait_for_count(&test_osk.shows, 1), "show did not reach Phosh");
	require(clock_gettime(CLOCK_MONOTONIC, &started) == 0, "clock failed");
	fb_hLinuxOskRequest(0);
	require(clock_gettime(CLOCK_MONOTONIC, &finished) == 0, "clock failed");
	elapsed_ns = (finished.tv_sec - started.tv_sec) * INT64_C(1000000000) +
		finished.tv_nsec - started.tv_nsec;
	require(elapsed_ns < INT64_C(500000000), "a slow show blocked the caller");
	pthread_mutex_lock(&test_osk.mutex);
	result = test_osk.hides == 0;
	test_osk.release_show = 1;
	pthread_cond_broadcast(&test_osk.changed);
	pthread_mutex_unlock(&test_osk.mutex);
	require(result, "hide overtook the in-flight show");
	require(wait_for_count(&test_osk.hides, 1), "hide did not reach Phosh");
	require(wait_for_worker(), "keyboard request worker did not finish");
	pthread_mutex_lock(&test_osk.mutex);
	result = test_osk.shows == 1 && !test_osk.visible;
	pthread_mutex_unlock(&test_osk.mutex);
	require(result, "the final keyboard state was not hidden");

	for (attempt = 0; attempt < 300; attempt++) {
		available = fb_hLinuxOskQuery(&visible);
		if (available == -1 && visible == 0)
			break;
		(void)nanosleep(&delay, NULL);
	}
	require(available == -1 && visible == 0,
		"the observed Phosh keyboard state was not cached");
	fb_hLinuxMaliitCancelPending();
	g_main_loop_quit(test_osk.loop);
	(void)pthread_join(server, NULL);
	g_test_dbus_down(bus);
	g_object_unref(bus);
	puts("LINUX_OSK_PHOSH_PEER_PASS");
	return EXIT_SUCCESS;
}

/* end of linux-osk-phosh-peer.c */
