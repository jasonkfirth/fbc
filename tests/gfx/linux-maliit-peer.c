/*
 * FreeBASIC Linux graphics tests
 * ------------------------------
 *
 * File: linux-maliit-peer.c
 *
 * Purpose:
 *
 *     Verify the Maliit bridge against a private GDBus session and peer.
 *
 * Responsibilities:
 *
 *     - publish a disposable Maliit-compatible address and input context
 *     - observe focused-field updates through the real asynchronous API
 *     - exercise deferred shows and cancellation against a mock focus service
 *     - check recovery after the input method's peer connection closes
 *     - send an input-context method as soon as each connection is accepted
 *     - check that a stalled peer cannot block queued show and hide requests
 *
 * Run from the repository root with the GIO development files installed:
 *
 *     cc -std=c11 -Wall -Wextra -Werror -pthread \
 *         tests/gfx/linux-maliit-peer.c \
 *         src/gfxlib2/drivers/linux/fb_linux_maliit.c \
 *         -o linux-maliit-peer $(pkg-config --cflags --libs gio-2.0) -ldl
 *
 * This file intentionally does NOT contain:
 *
 *     - a desktop session, keyboard layout, or application text
 *     - access to the user's session bus
 */

#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <gio/gio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../src/gfxlib2/drivers/linux/fb_linux_maliit.h"

typedef struct TEST_PEER {
	pthread_mutex_t mutex;
	pthread_cond_t changed;
	int ready;
	int failed;
	int updates;
	int shows;
	int hides;
	int focused;
	int focus_queries;
	int connections;
	int early_completed;
	int early_failed;
	int received_keys;
	uint32_t last_key;
	FB_LINUX_MALIIT_INPUT_KIND last_kind;
	int block_show;
	int x, y, width, height;
	guint64 window_id;
	GMainLoop *loop;
	GDBusServer *server;
	GDBusConnection *session;
	GDBusConnection *peer;
	GDBusNodeInfo *address_info;
	GDBusNodeInfo *input_info;
	GDBusNodeInfo *focus_info;
	unsigned int address_registration;
	unsigned int focus_registration;
} TEST_PEER;

static TEST_PEER test_peer = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.changed = PTHREAD_COND_INITIALIZER,
	.focused = 1
};

static void receive_key(void *user_data, uint32_t key,
	FB_LINUX_MALIIT_INPUT_KIND kind)
{
	TEST_PEER *state = user_data;
	pthread_mutex_lock(&state->mutex);
	state->last_key = key;
	state->last_kind = kind;
	state->received_keys++;
	pthread_cond_broadcast(&state->changed);
	pthread_mutex_unlock(&state->mutex);
}

static const char address_xml[] =
	"<node><interface name='org.maliit.Server.Address'>"
	"<property name='address' type='s' access='read'/>"
	"</interface></node>";

static const char input_xml[] =
	"<node><interface name='com.meego.inputmethod.uiserver1'>"
	"<method name='activateContext'/>"
	"<method name='updateWidgetInformation'>"
	"<arg type='a{sv}' direction='in'/><arg type='b' direction='in'/>"
	"</method>"
	"<method name='showInputMethod'/><method name='hideInputMethod'/>"
	"</interface></node>";

static const char focus_xml[] =
	"<node><interface name='com.lomiri.Shell.FocusInfo'>"
	"<method name='isPidFocused'><arg type='u' direction='in'/>"
	"<arg type='b' direction='out'/></method></interface></node>";

static void focus_call(GDBusConnection *connection, const char *sender,
	const char *object_path, const char *interface_name, const char *method,
	GVariant *parameters, GDBusMethodInvocation *invocation, void *user_data)
{
	TEST_PEER *state = user_data;
	int focused;
	(void)connection;
	(void)sender;
	(void)object_path;
	(void)interface_name;
	(void)method;
	(void)parameters;
	pthread_mutex_lock(&state->mutex);
	focused = state->focused;
	state->focus_queries++;
	pthread_cond_broadcast(&state->changed);
	pthread_mutex_unlock(&state->mutex);
	g_dbus_method_invocation_return_value(invocation,
		g_variant_new("(b)", focused));
}

static GVariant *get_address(GDBusConnection *connection, const char *sender,
	const char *object_path, const char *interface_name,
	const char *property_name, GError **error, void *user_data)
{
	TEST_PEER *state = user_data;
	(void)connection;
	(void)sender;
	(void)object_path;
	(void)interface_name;
	(void)error;
	if (strcmp(property_name, "address") != 0)
		return NULL;
	return g_variant_new_string(g_dbus_server_get_client_address(state->server));
}

static void input_call(GDBusConnection *connection, const char *sender,
	const char *object_path, const char *interface_name, const char *method,
	GVariant *parameters, GDBusMethodInvocation *invocation, void *user_data)
{
	TEST_PEER *state = user_data;
	(void)connection;
	(void)sender;
	(void)object_path;
	(void)interface_name;
	if (strcmp(method, "showInputMethod") == 0 ||
	    strcmp(method, "hideInputMethod") == 0) {
		pthread_mutex_lock(&state->mutex);
		if (strcmp(method, "showInputMethod") == 0)
			state->shows++;
		else
			state->hides++;
		pthread_cond_broadcast(&state->changed);
		while (strcmp(method, "showInputMethod") == 0 && state->block_show)
			pthread_cond_wait(&state->changed, &state->mutex);
		pthread_mutex_unlock(&state->mutex);
	}
	if (strcmp(method, "updateWidgetInformation") == 0) {
		GVariant *dictionary = NULL;
		gboolean focus_changed;
		int x, y, width, height;
		guint64 window_id;
		g_variant_get(parameters, "(@a{sv}b)", &dictionary, &focus_changed);
		if (g_variant_lookup(dictionary, "cursorRectangle", "(iiii)",
			&x, &y, &width, &height) &&
		    g_variant_lookup(dictionary, "winId", "t", &window_id)) {
			pthread_mutex_lock(&state->mutex);
			state->x = x;
			state->y = y;
			state->width = width;
			state->height = height;
			state->window_id = window_id;
			state->updates++;
			pthread_cond_broadcast(&state->changed);
			pthread_mutex_unlock(&state->mutex);
		}
		(void)focus_changed;
		g_variant_unref(dictionary);
	}
	g_dbus_method_invocation_return_value(invocation, g_variant_new("()"));
}

static void early_commit_done(GObject *source, GAsyncResult *result,
	void *user_data)
{
	TEST_PEER *state = user_data;
	GError *error = NULL;
	GVariant *reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source),
		result, &error);
	pthread_mutex_lock(&state->mutex);
	state->early_completed++;
	if (!reply)
		state->early_failed++;
	pthread_cond_broadcast(&state->changed);
	pthread_mutex_unlock(&state->mutex);
	if (reply)
		g_variant_unref(reply);
	if (error)
		g_error_free(error);
}

static gboolean accept_peer(GDBusServer *server, GDBusConnection *connection,
	void *user_data)
{
	static const GDBusInterfaceVTable input_vtable = {
		.method_call = input_call
	};
	TEST_PEER *state = user_data;
	GError *error = NULL;
	unsigned int registration;
	(void)server;
	registration = g_dbus_connection_register_object(connection,
		"/com/meego/inputmethod/uiserver1", state->input_info->interfaces[0],
		&input_vtable, state, NULL, &error);
	if (!registration) {
		if (error)
			g_error_free(error);
		return FALSE;
	}
	pthread_mutex_lock(&state->mutex);
	if (state->peer)
		g_object_unref(state->peer);
	state->peer = g_object_ref(connection);
	state->connections++;
	pthread_cond_broadcast(&state->changed);
	pthread_mutex_unlock(&state->mutex);
	/* Maliit may send a method immediately after accepting a new peer. */
	g_dbus_connection_call(connection, NULL, "/com/meego/inputmethod/inputcontext",
		"com.meego.inputmethod.inputcontext1", "commitString",
		g_variant_new("(siii)", "", 0, 0, 0), G_VARIANT_TYPE_UNIT,
		G_DBUS_CALL_FLAGS_NONE, 2000, NULL, early_commit_done, state);
	return TRUE;
}

static void *run_peer(void *unused)
{
	static const GDBusInterfaceVTable address_vtable = {
		.get_property = get_address
	};
	static const GDBusInterfaceVTable focus_vtable = {
		.method_call = focus_call
	};
	GMainContext *context = g_main_context_new();
	GError *error = NULL;
	GVariant *reply = NULL;
	char *guid = g_dbus_generate_guid();
	(void)unused;
	g_main_context_push_thread_default(context);
	test_peer.server = g_dbus_server_new_sync("unix:tmpdir=/tmp",
		G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS, guid,
		NULL, NULL, &error);
	g_free(guid);
	if (!test_peer.server)
		goto finished;
	test_peer.address_info = g_dbus_node_info_new_for_xml(address_xml, &error);
	if (!test_peer.address_info)
		goto finished;
	test_peer.input_info = g_dbus_node_info_new_for_xml(input_xml, &error);
	if (!test_peer.input_info)
		goto finished;
	test_peer.focus_info = g_dbus_node_info_new_for_xml(focus_xml, &error);
	if (!test_peer.focus_info)
		goto finished;
	test_peer.session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	if (!test_peer.session)
		goto finished;
	test_peer.address_registration = g_dbus_connection_register_object(
		test_peer.session, "/org/maliit/server/address",
		test_peer.address_info->interfaces[0], &address_vtable,
		&test_peer, NULL, &error);
	if (!test_peer.address_registration)
		goto finished;
	test_peer.focus_registration = g_dbus_connection_register_object(
		test_peer.session, "/com/lomiri/Shell/FocusInfo",
		test_peer.focus_info->interfaces[0], &focus_vtable,
		&test_peer, NULL, &error);
	if (!test_peer.focus_registration)
		goto finished;
	reply = g_dbus_connection_call_sync(test_peer.session,
		"org.freedesktop.DBus", "/org/freedesktop/DBus",
		"org.freedesktop.DBus", "RequestName",
		g_variant_new("(su)", "org.maliit.server", 0u), NULL,
		G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &error);
	if (!reply)
		goto finished;
	g_variant_unref(reply);
	reply = g_dbus_connection_call_sync(test_peer.session,
		"org.freedesktop.DBus", "/org/freedesktop/DBus",
		"org.freedesktop.DBus", "RequestName",
		g_variant_new("(su)", "com.lomiri.Shell.FocusInfo", 0u), NULL,
		G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &error);
	if (!reply)
		goto finished;
	g_variant_unref(reply);
	g_signal_connect(test_peer.server, "new-connection",
		G_CALLBACK(accept_peer), &test_peer);
	g_dbus_server_start(test_peer.server);
	test_peer.loop = g_main_loop_new(context, FALSE);
	if (!test_peer.loop)
		goto finished;
	pthread_mutex_lock(&test_peer.mutex);
	test_peer.ready = 1;
	pthread_cond_broadcast(&test_peer.changed);
	pthread_mutex_unlock(&test_peer.mutex);
	g_main_loop_run(test_peer.loop);

finished:
	if (error) {
		fprintf(stderr, "peer setup: %s\n", error->message);
		g_error_free(error);
	}
	pthread_mutex_lock(&test_peer.mutex);
	if (!test_peer.ready)
		test_peer.failed = 1;
	pthread_cond_broadcast(&test_peer.changed);
	pthread_mutex_unlock(&test_peer.mutex);
	if (test_peer.address_registration)
		g_dbus_connection_unregister_object(test_peer.session,
			test_peer.address_registration);
	if (test_peer.focus_registration)
		g_dbus_connection_unregister_object(test_peer.session,
			test_peer.focus_registration);
	if (test_peer.peer)
		g_object_unref(test_peer.peer);
	if (test_peer.session)
		g_object_unref(test_peer.session);
	if (test_peer.server) {
		g_dbus_server_stop(test_peer.server);
		g_object_unref(test_peer.server);
	}
	if (test_peer.address_info)
		g_dbus_node_info_unref(test_peer.address_info);
	if (test_peer.input_info)
		g_dbus_node_info_unref(test_peer.input_info);
	if (test_peer.focus_info)
		g_dbus_node_info_unref(test_peer.focus_info);
	if (test_peer.loop)
		g_main_loop_unref(test_peer.loop);
	g_main_context_pop_thread_default(context);
	g_main_context_unref(context);
	return NULL;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "%s\n", message);
		exit(1);
	}
}

static void send_peer_method(const char *method, GVariant *parameters)
{
	GDBusConnection *peer;
	GError *error = NULL;
	GVariant *reply;
	pthread_mutex_lock(&test_peer.mutex);
	peer = test_peer.peer ? g_object_ref(test_peer.peer) : NULL;
	pthread_mutex_unlock(&test_peer.mutex);
	require(peer != NULL, "missing peer for input method call");
	reply = g_dbus_connection_call_sync(peer, NULL,
		"/com/meego/inputmethod/inputcontext",
		"com.meego.inputmethod.inputcontext1", method, parameters, NULL,
		G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &error);
	g_object_unref(peer);
	if (error)
		g_error_free(error);
	require(reply != NULL, "input method callback failed");
	g_variant_unref(reply);
}

static int wait_for_count(int *count, int target)
{
	struct timespec deadline;
	int reached;

	if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
		return 0;
	deadline.tv_sec += 3;
	pthread_mutex_lock(&test_peer.mutex);
	while (*count < target) {
		if (pthread_cond_timedwait(&test_peer.changed, &test_peer.mutex,
			&deadline) != 0)
			break;
	}
	reached = *count >= target;
	pthread_mutex_unlock(&test_peer.mutex);
	return reached;
}

/* A missing procfs should not prevent the behavioral test from running. */
static int count_entries(const char *path)
{
	DIR *directory = opendir(path);
	struct dirent *entry;
	int count = 0;
	if (!directory)
		return -1;
	while ((entry = readdir(directory)) != NULL) {
		if (strcmp(entry->d_name, ".") != 0 &&
		    strcmp(entry->d_name, "..") != 0)
			count++;
	}
	closedir(directory);
	return count;
}

int main(void)
{
	GTestDBus *bus;
	pthread_t worker;
	struct timespec deadline;
	const struct timespec retry_delay = { 0, 750000000L };
	const struct timespec disconnect_delay = { 0, 20000000L };
	const struct timespec cleanup_delay = { 0, 200000000L };
	int query_count, show_count, hide_count, key_count;
	int result, i, attempt, visible;
	int before_fds = -1, before_threads = -1;

	require(setenv("XDG_CURRENT_DESKTOP", "Lomiri", 1) == 0, "setenv failed");
	require(setenv("WAYLAND_DISPLAY", "test-wayland", 1) == 0, "setenv failed");
	bus = g_test_dbus_new(G_TEST_DBUS_NONE);
	g_test_dbus_up(bus);
	require(pthread_create(&worker, NULL, run_peer, NULL) == 0,
		"peer thread failed");
	pthread_mutex_lock(&test_peer.mutex);
	while (!test_peer.ready && !test_peer.failed)
		pthread_cond_wait(&test_peer.changed, &test_peer.mutex);
	result = test_peer.ready;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "peer did not start");

	fb_hLinuxMaliitSetKeyCallback(receive_key, &test_peer);
	fb_hLinuxMaliitSetTextRegion(1, 2, 3, 4, 10);
	fb_hLinuxMaliitSetWindowId(99);
	require(fb_hLinuxMaliitRequestKeyboard(1) == 1, "show request failed");
	require(wait_for_count(&test_peer.early_completed, 1),
		"the peer's early input-context call did not finish");
	pthread_mutex_lock(&test_peer.mutex);
	result = test_peer.early_failed == 0;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "the peer's early input-context call failed");
	pthread_mutex_lock(&test_peer.mutex);
	result = test_peer.updates == 1 && test_peer.window_id == 99;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "show did not include the current window id");
	/* Unicode text can have the same numeric value as an extended key. */
	pthread_mutex_lock(&test_peer.mutex);
	key_count = test_peer.received_keys;
	pthread_mutex_unlock(&test_peer.mutex);
	send_peer_method("commitString", g_variant_new("(siii)", "\xe4\xaf\xbf", 0, 0, 0));
	require(wait_for_count(&test_peer.received_keys, key_count + 1),
		"Unicode text did not reach the callback");
	pthread_mutex_lock(&test_peer.mutex);
	result = test_peer.last_key == 0x4bffu &&
		test_peer.last_kind == FB_LINUX_MALIIT_TEXT;
	key_count = test_peer.received_keys;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "Unicode text was mistaken for an extended key");
	send_peer_method("keyEvent", g_variant_new("(iiisbiy)",
		6, 0x01000012, 0, "", FALSE, 1, (guchar)0));
	require(wait_for_count(&test_peer.received_keys, key_count + 1),
		"extended key did not reach the callback");
	pthread_mutex_lock(&test_peer.mutex);
	result = test_peer.last_key == 0x4bffu &&
		test_peer.last_kind == FB_LINUX_MALIIT_KEY;
	key_count = test_peer.received_keys;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "extended key was mistaken for Unicode text");
	send_peer_method("keyEvent", g_variant_new("(iiisbiy)",
		6, 0x01000004, 0, "", FALSE, 1, (guchar)0));
	require(wait_for_count(&test_peer.received_keys, key_count + 1),
		"Enter did not reach the callback");
	pthread_mutex_lock(&test_peer.mutex);
	result = test_peer.last_key == '\r' &&
		test_peer.last_kind == FB_LINUX_MALIIT_KEY;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "Enter key was decoded incorrectly");
	fb_hLinuxMaliitSetTextRegion(11, 22, 33, 44, 101);
	require(clock_gettime(CLOCK_REALTIME, &deadline) == 0, "clock failed");
	deadline.tv_sec += 3;
	pthread_mutex_lock(&test_peer.mutex);
	while (test_peer.updates < 2) {
		if (pthread_cond_timedwait(&test_peer.changed, &test_peer.mutex,
			&deadline) != 0)
			break;
	}
	result = test_peer.updates >= 2 && test_peer.x == 11 &&
		test_peer.y == 22 && test_peer.width == 33 &&
		test_peer.height == 44 && test_peer.window_id == 101;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "peer did not receive the changed region");
	require(fb_hLinuxMaliitRequestKeyboard(0) == 1, "hide request failed");

	/* The worker coalesces newer requests while a peer call is stalled. */
	{
		struct timespec started, finished;
		int64_t elapsed_ns;
		pthread_mutex_lock(&test_peer.mutex);
		test_peer.focused = 1;
		test_peer.block_show = 1;
		show_count = test_peer.shows;
		hide_count = test_peer.hides;
		pthread_mutex_unlock(&test_peer.mutex);
		require(fb_hLinuxMaliitQueueKeyboard(1, NULL) == 1,
			"asynchronous show was not queued");
		require(wait_for_count(&test_peer.shows, show_count + 1),
			"asynchronous show did not reach the peer");
		require(clock_gettime(CLOCK_MONOTONIC, &started) == 0, "clock failed");
		require(fb_hLinuxMaliitQueueKeyboard(0, NULL) == 1 &&
			fb_hLinuxMaliitQueueKeyboard(1, NULL) == 1 &&
			fb_hLinuxMaliitQueueKeyboard(0, NULL) == 1,
			"rapid requests were not queued");
		require(clock_gettime(CLOCK_MONOTONIC, &finished) == 0, "clock failed");
		elapsed_ns = (finished.tv_sec - started.tv_sec) * INT64_C(1000000000) +
			finished.tv_nsec - started.tv_nsec;
		require(elapsed_ns < INT64_C(500000000),
			"a stalled peer blocked the requesting thread");
		pthread_mutex_lock(&test_peer.mutex);
		test_peer.block_show = 0;
		pthread_cond_broadcast(&test_peer.changed);
		pthread_mutex_unlock(&test_peer.mutex);
		require(wait_for_count(&test_peer.hides, hide_count + 1),
			"the final hide did not reach the peer");
		(void)nanosleep(&retry_delay, NULL);
		pthread_mutex_lock(&test_peer.mutex);
		result = test_peer.shows == show_count + 1;
		pthread_mutex_unlock(&test_peer.mutex);
		require(result, "an older queued show reopened the keyboard");
	}

	/* A timed-out show may have been delivered before the mode closed. */
	{
		const struct timespec show_timeout = { 1, 300000000L };
		pthread_mutex_lock(&test_peer.mutex);
		test_peer.block_show = 1;
		show_count = test_peer.shows;
		hide_count = test_peer.hides;
		pthread_mutex_unlock(&test_peer.mutex);
		require(fb_hLinuxMaliitQueueKeyboard(1, NULL) == 1,
			"timeout test show was not queued");
		require(wait_for_count(&test_peer.shows, show_count + 1),
			"timeout test show did not reach the peer");
		fb_hLinuxMaliitCancelPending();
		(void)nanosleep(&show_timeout, NULL);
		pthread_mutex_lock(&test_peer.mutex);
		test_peer.block_show = 0;
		pthread_cond_broadcast(&test_peer.changed);
		pthread_mutex_unlock(&test_peer.mutex);
		require(wait_for_count(&test_peer.hides, hide_count + 1),
			"a timed-out show was not hidden after close");
	}

	fb_hLinuxMaliitSetKeyCallback(receive_key, &test_peer);

	/* A show before the compositor grants focus must wait, then happen once. */
	pthread_mutex_lock(&test_peer.mutex);
	test_peer.focused = 0;
	query_count = test_peer.focus_queries;
	show_count = test_peer.shows;
	pthread_mutex_unlock(&test_peer.mutex);
	require(fb_hLinuxMaliitRequestKeyboard(1) == 1, "deferred show was not queued");
	require(wait_for_count(&test_peer.focus_queries, query_count + 1),
		"focus service was not queried");
	pthread_mutex_lock(&test_peer.mutex);
	result = test_peer.shows == show_count;
	test_peer.focused = 1;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "keyboard was shown before the window had focus");
	require(wait_for_count(&test_peer.shows, show_count + 1),
		"keyboard was not shown after focus arrived");
	require(fb_hLinuxMaliitRequestKeyboard(0) == 1, "hide after retry failed");

	/* Hiding or closing the window must cancel a pending show. */
	pthread_mutex_lock(&test_peer.mutex);
	test_peer.focused = 0;
	query_count = test_peer.focus_queries;
	show_count = test_peer.shows;
	pthread_mutex_unlock(&test_peer.mutex);
	require(fb_hLinuxMaliitRequestKeyboard(1) == 1, "second show was not queued");
	require(wait_for_count(&test_peer.focus_queries, query_count + 1),
		"second focus check was not made");
	require(fb_hLinuxMaliitRequestKeyboard(0) == 1, "pending show hide failed");
	pthread_mutex_lock(&test_peer.mutex);
	test_peer.focused = 1;
	pthread_mutex_unlock(&test_peer.mutex);
	(void)nanosleep(&retry_delay, NULL);
	pthread_mutex_lock(&test_peer.mutex);
	result = test_peer.shows == show_count;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "a hidden keyboard reopened after focus arrived");

	pthread_mutex_lock(&test_peer.mutex);
	test_peer.focused = 0;
	query_count = test_peer.focus_queries;
	pthread_mutex_unlock(&test_peer.mutex);
	require(fb_hLinuxMaliitRequestKeyboard(1) == 1, "window-close show was not queued");
	require(wait_for_count(&test_peer.focus_queries, query_count + 1),
		"window-close focus check was not made");
	fb_hLinuxMaliitCancelPending();
	pthread_mutex_lock(&test_peer.mutex);
	test_peer.focused = 1;
	pthread_mutex_unlock(&test_peer.mutex);
	(void)nanosleep(&retry_delay, NULL);
	pthread_mutex_lock(&test_peer.mutex);
	result = test_peer.shows == show_count;
	pthread_mutex_unlock(&test_peer.mutex);
	require(result, "a closed window reopened the keyboard");

	/* If a window closes during the peer call, undo that in-flight show. */
	pthread_mutex_lock(&test_peer.mutex);
	test_peer.focused = 0;
	test_peer.block_show = 1;
	query_count = test_peer.focus_queries;
	hide_count = test_peer.hides;
	pthread_mutex_unlock(&test_peer.mutex);
	require(fb_hLinuxMaliitRequestKeyboard(1) == 1, "in-flight show was not queued");
	require(wait_for_count(&test_peer.focus_queries, query_count + 1),
		"in-flight focus check was not made");
	pthread_mutex_lock(&test_peer.mutex);
	test_peer.focused = 1;
	pthread_mutex_unlock(&test_peer.mutex);
	require(wait_for_count(&test_peer.shows, show_count + 1),
		"in-flight show did not reach the peer");
	fb_hLinuxMaliitCancelPending();
	pthread_mutex_lock(&test_peer.mutex);
	test_peer.block_show = 0;
	pthread_cond_broadcast(&test_peer.changed);
	pthread_mutex_unlock(&test_peer.mutex);
	require(wait_for_count(&test_peer.hides, hide_count + 1),
		"in-flight show was not hidden after window close");
	require(fb_hLinuxMaliitRequestKeyboard(0) == 1, "final hide failed");

	/* A reopened mode registers its input callback before requesting input. */
	fb_hLinuxMaliitSetKeyCallback(receive_key, &test_peer);

	/* A restarted input method closes its old peer and publishes a new one. */
	for (i = 0; i < 10; i++) {
		GDBusConnection *peer;
		GError *error = NULL;
		GVariant *reply;
		pthread_mutex_lock(&test_peer.mutex);
		peer = test_peer.peer ? g_object_ref(test_peer.peer) : NULL;
		show_count = test_peer.shows;
		key_count = test_peer.received_keys;
		pthread_mutex_unlock(&test_peer.mutex);
		require(peer != NULL, "missing peer connection");
		result = g_dbus_connection_close_sync(peer, NULL, &error);
		if (error)
			g_error_free(error);
		g_object_unref(peer);
		require(result, "could not close peer connection");
		for (attempt = 0; attempt < 150; attempt++) {
			if (!fb_hLinuxMaliitQueryKeyboard(&visible))
				break;
			(void)nanosleep(&disconnect_delay, NULL);
		}
		require(attempt < 150, "keyboard bridge did not notice peer disconnect");
		require(fb_hLinuxMaliitRequestKeyboard(1) == 1,
			"show after peer restart failed");
		require(wait_for_count(&test_peer.connections, i + 2),
			"peer connection was not renewed");
		require(wait_for_count(&test_peer.early_completed, i + 2),
			"the renewed peer's early call did not finish");
		pthread_mutex_lock(&test_peer.mutex);
		result = test_peer.early_failed == 0;
		pthread_mutex_unlock(&test_peer.mutex);
		require(result, "a renewed peer's early call failed");
		require(wait_for_count(&test_peer.shows, show_count + 1),
			"renewed peer did not receive show");
		pthread_mutex_lock(&test_peer.mutex);
		peer = test_peer.peer ? g_object_ref(test_peer.peer) : NULL;
		result = test_peer.window_id == 101 && test_peer.x == 11 &&
			test_peer.y == 22 && test_peer.width == 33 && test_peer.height == 44;
		pthread_mutex_unlock(&test_peer.mutex);
		require(peer != NULL && result, "renewed peer lost the text region");
		reply = g_dbus_connection_call_sync(peer, NULL,
			"/com/meego/inputmethod/inputcontext",
			"com.meego.inputmethod.inputcontext1", "commitString",
			g_variant_new("(siii)", "a", 0, 0, 0), NULL,
			G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &error);
		g_object_unref(peer);
		if (error)
			g_error_free(error);
		require(reply != NULL, "renewed peer could not commit text");
		g_variant_unref(reply);
		require(wait_for_count(&test_peer.received_keys, key_count + 1),
			"committed text did not reach the graphics input callback");
		pthread_mutex_lock(&test_peer.mutex);
		result = test_peer.last_key == 'a' &&
			test_peer.last_kind == FB_LINUX_MALIIT_TEXT;
		pthread_mutex_unlock(&test_peer.mutex);
		require(result, "committed text reached the callback with the wrong key");
		require(fb_hLinuxMaliitRequestKeyboard(0) == 1,
			"hide after peer restart failed");
		if (i == 0 || i == 9) {
			int fds, threads;
			(void)nanosleep(&cleanup_delay, NULL);
			fds = count_entries("/proc/self/fd");
			threads = count_entries("/proc/self/task");
			if (i == 0) {
				before_fds = fds;
				before_threads = threads;
			} else {
				/* Allow two transient GLib resources, but not one per restart. */
				if (before_fds >= 0 && fds >= 0)
					require(fds <= before_fds + 2,
						"peer restarts left file descriptors behind");
				if (before_threads >= 0 && threads >= 0)
					require(threads <= before_threads + 2,
						"peer restarts left worker threads behind");
			}
		}
	}

	/* The old connection must also survive until an in-flight call returns. */
	{
		GDBusConnection *peer;
		GError *error = NULL;
		pthread_mutex_lock(&test_peer.mutex);
		test_peer.focused = 0;
		test_peer.block_show = 1;
		query_count = test_peer.focus_queries;
		show_count = test_peer.shows;
		pthread_mutex_unlock(&test_peer.mutex);
		require(fb_hLinuxMaliitRequestKeyboard(1) == 1,
			"restart-in-flight show was not queued");
		require(wait_for_count(&test_peer.focus_queries, query_count + 1),
			"restart-in-flight focus check was not made");
		pthread_mutex_lock(&test_peer.mutex);
		test_peer.focused = 1;
		pthread_mutex_unlock(&test_peer.mutex);
		require(wait_for_count(&test_peer.shows, show_count + 1),
			"restart-in-flight show did not reach the peer");
		pthread_mutex_lock(&test_peer.mutex);
		peer = test_peer.peer ? g_object_ref(test_peer.peer) : NULL;
		pthread_mutex_unlock(&test_peer.mutex);
		require(peer != NULL, "missing in-flight peer connection");
		result = g_dbus_connection_close_sync(peer, NULL, &error);
		if (error)
			g_error_free(error);
		g_object_unref(peer);
		pthread_mutex_lock(&test_peer.mutex);
		test_peer.block_show = 0;
		pthread_cond_broadcast(&test_peer.changed);
		pthread_mutex_unlock(&test_peer.mutex);
		require(result, "could not close in-flight peer connection");
		for (attempt = 0; attempt < 150; attempt++) {
			if (!fb_hLinuxMaliitQueryKeyboard(&visible))
				break;
			(void)nanosleep(&disconnect_delay, NULL);
		}
		require(attempt < 150, "in-flight disconnect was not noticed");
		require(fb_hLinuxMaliitRequestKeyboard(1) == 1,
			"show after in-flight disconnect failed");
		require(wait_for_count(&test_peer.shows, show_count + 2),
			"new peer did not receive show after in-flight disconnect");
		require(fb_hLinuxMaliitRequestKeyboard(0) == 1,
			"hide after in-flight disconnect failed");
	}

	fb_hLinuxMaliitSetKeyCallback(NULL, NULL);
	g_main_loop_quit(test_peer.loop);
	pthread_join(worker, NULL);
	g_test_dbus_down(bus);
	g_object_unref(bus);
	puts("linux-maliit-peer: passed");
	return 0;
}

/* end of linux-maliit-peer.c */
