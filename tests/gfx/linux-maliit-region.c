/*
 * FreeBASIC Linux graphics tests
 * ------------------------------
 *
 * File: linux-maliit-region.c
 *
 * Purpose:
 *
 *     Check that a focused input context publishes changing cursor bounds.
 *
 * Responsibilities:
 *
 *     - exercise the real Maliit bridge with recorded GDBus calls
 *     - verify the rectangle and window id delivered to the peer
 *
 * Run from the repository root with the GIO development files installed:
 *
 *     cc -std=c11 -Wall -Wextra -Werror -pthread \
 *         tests/gfx/linux-maliit-region.c -o linux-maliit-region \
 *         $(pkg-config --cflags --libs gio-2.0) -ldl
 *
 * This file intentionally does NOT contain:
 *
 *     - a desktop session or a physical keyboard
 *     - a replacement implementation of Maliit's wire protocol
 */

#define _POSIX_C_SOURCE 200809L

#include <gio/gio.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/gfxlib2/drivers/linux/fb_linux_maliit.c"

static int update_count;
static int last_x, last_y, last_width, last_height;
static guint64 last_window_id;
static int change_during_show;
static int lose_during_show;
static int area_during_show;
static int peer_references;

static void *record_peer_ref(void *object)
{
	if (object != (void *)(uintptr_t)1) {
		fprintf(stderr, "unexpected peer reference\n");
		exit(1);
	}
	peer_references++;
	return object;
}

static void record_peer_unref(void *object)
{
	if (object != (void *)(uintptr_t)1 || peer_references <= 0) {
		fprintf(stderr, "unbalanced peer reference\n");
		exit(1);
	}
	peer_references--;
}

static void record_widget_state(GVariant *parameters)
{
	GVariant *dictionary = NULL;
	gboolean focus_changed;
	gboolean prediction_enabled = TRUE;

	g_variant_ref_sink(parameters);
	g_variant_get(parameters, "(@a{sv}b)", &dictionary, &focus_changed);
	if (!g_variant_lookup(dictionary, "predictionEnabled", "b", &prediction_enabled) ||
	    prediction_enabled) {
		fprintf(stderr, "keyboard was not asked to use committed text\n");
		exit(1);
	}
	if (!g_variant_lookup(dictionary, "cursorRectangle", "(iiii)",
		&last_x, &last_y, &last_width, &last_height) ||
	    !g_variant_lookup(dictionary, "winId", "t", &last_window_id)) {
		fprintf(stderr, "missing widget state fields\n");
		exit(1);
	}
	(void)focus_changed;
	update_count++;
	g_variant_unref(dictionary);
	g_variant_unref(parameters);
}

static GVariant *record_call_sync(GDBusConnection *connection,
	const char *bus_name, const char *object_path, const char *interface_name,
	const char *method_name, GVariant *parameters,
	const GVariantType *reply_type, int flags, int timeout_msec,
	void *cancellable, GError **error)
{
	(void)connection;
	(void)bus_name;
	(void)object_path;
	(void)interface_name;
	(void)reply_type;
	(void)flags;
	(void)timeout_msec;
	(void)cancellable;
	(void)error;
	if (strcmp(method_name, "updateWidgetInformation") == 0)
		record_widget_state(parameters);
	else
		g_variant_unref(g_variant_ref_sink(parameters));
	if (strcmp(method_name, "showInputMethod") == 0) {
		if (change_during_show)
			fb_hLinuxMaliitSetTextRegion(50, 60, 70, 80, 102);
		if (lose_during_show)
			fb_hLinuxMaliitMethodCall(NULL, NULL, NULL, NULL,
				"activationLostEvent", NULL,
				(GDBusMethodInvocation *)(uintptr_t)1, NULL);
		if (area_during_show) {
			GVariant *area = g_variant_ref_sink(
				g_variant_new("(iiii)", 0, 400, 360, 240));
			fb_hLinuxMaliitMethodCall(NULL, NULL, NULL, NULL,
				"updateInputMethodArea", area,
				(GDBusMethodInvocation *)(uintptr_t)1, NULL);
			g_variant_unref(area);
		}
	}
	return g_variant_ref_sink(g_variant_new("()"));
}

static void record_call_async(GDBusConnection *connection,
	const char *bus_name, const char *object_path, const char *interface_name,
	const char *method_name, GVariant *parameters,
	const GVariantType *reply_type, int flags, int timeout_msec,
	GCancellable *cancellable, FB_GASYNC_READY_CALLBACK callback, void *user_data)
{
	(void)connection;
	(void)bus_name;
	(void)object_path;
	(void)interface_name;
	(void)reply_type;
	(void)flags;
	(void)timeout_msec;
	(void)cancellable;
	(void)user_data;
	if (strcmp(method_name, "updateWidgetInformation") != 0 || callback) {
		fprintf(stderr, "unexpected asynchronous request\n");
		exit(1);
	}
	record_widget_state(parameters);
}

static void record_method_return(GDBusMethodInvocation *invocation,
	GVariant *parameters)
{
	(void)invocation;
	g_variant_unref(g_variant_ref_sink(parameters));
}

static void check(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "%s\n", message);
		exit(1);
	}
}

static void check_area(int x, int y, int width, int height,
	int expected_top, int expected_height)
{
	int top = -7, overlap = -9;
	GVariant *area = g_variant_ref_sink(
		g_variant_new("(iiii)", x, y, width, height));
	fb_hLinuxMaliitMethodCall(NULL, NULL, NULL, NULL,
		"updateInputMethodArea", area,
		(GDBusMethodInvocation *)(uintptr_t)1, NULL);
	g_variant_unref(area);
	int result = fb_hLinuxMaliitGetKeyboardOcclusion(1280, 800, &top, &overlap);
	if (expected_height > 0)
		check(result == 1 && top == expected_top && overlap == expected_height,
			"incorrect decoded Maliit keyboard intersection");
	else
		check(result == 0 && top == -7 && overlap == -9,
			"absent Maliit bounds changed the outputs");
}

int main(void)
{
	int visible;
	int before;
	check(setenv("XDG_CURRENT_DESKTOP", "Lomiri", 1) == 0, "setenv failed");
	fb_hLinuxMaliitApi.connection_call_sync = record_call_sync;
	fb_hLinuxMaliitApi.connection_call = record_call_async;
	fb_hLinuxMaliitApi.variant_builder_new = g_variant_builder_new;
	fb_hLinuxMaliitApi.variant_builder_add_value = g_variant_builder_add_value;
	fb_hLinuxMaliitApi.variant_builder_end = g_variant_builder_end;
	fb_hLinuxMaliitApi.variant_builder_unref = g_variant_builder_unref;
	fb_hLinuxMaliitApi.variant_new = g_variant_new;
	fb_hLinuxMaliitApi.variant_new_string = g_variant_new_string;
	fb_hLinuxMaliitApi.variant_new_boolean = g_variant_new_boolean;
	fb_hLinuxMaliitApi.variant_new_int32 = g_variant_new_int32;
	fb_hLinuxMaliitApi.variant_new_uint64 = g_variant_new_uint64;
	fb_hLinuxMaliitApi.variant_new_variant = g_variant_new_variant;
	fb_hLinuxMaliitApi.variant_new_dict_entry = g_variant_new_dict_entry;
	fb_hLinuxMaliitApi.variant_get = g_variant_get;
	fb_hLinuxMaliitApi.variant_unref = g_variant_unref;
	fb_hLinuxMaliitApi.method_invocation_return_value = record_method_return;
	fb_hLinuxMaliitApi.object_ref = record_peer_ref;
	fb_hLinuxMaliitApi.object_unref = record_peer_unref;
	fb_hLinuxMaliitState.initialization = 2;
	fb_hLinuxMaliitState.available = 1;
	fb_hLinuxMaliitState.connection = (GDBusConnection *)(uintptr_t)1;
	check(fb_hLinuxMaliitCall("invalid", NULL) == NULL && peer_references == 0,
		"a failed call retained the peer");

	fb_hLinuxMaliitSetTextRegion(10, 20, 30, 40, 100);
	check(update_count == 0, "inactive region sent an update");
	check(fb_hLinuxMaliitRequestKeyboard(1) == 1, "show request failed");
	check(update_count == 1 && last_x == 10 && last_window_id == 100,
		"show did not send the current region");
	fb_hLinuxMaliitSetTextRegion(10, 20, 30, 40, 100);
	check(update_count == 1, "unchanged region sent another update");
	fb_hLinuxMaliitSetTextRegion(11, 22, 33, 44, 101);
	check(update_count == 2 && last_x == 11 && last_y == 22 &&
		last_width == 33 && last_height == 44 && last_window_id == 101,
		"active region change was not delivered");
	fb_hLinuxMaliitSetTextRegion(-1, 22, 33, 44, 101);
	fb_hLinuxMaliitSetTextRegion(11, 22, 0, 44, 101);
	check(update_count == 2, "invalid region sent an update");
	check(fb_hLinuxMaliitRequestKeyboard(0) == 1, "hide request failed");
	fb_hLinuxMaliitSetTextRegion(12, 23, 34, 45, 101);
	check(update_count == 2, "hidden context sent an update");
	change_during_show = 1;
	check(fb_hLinuxMaliitRequestKeyboard(1) == 1, "second show request failed");
	check(update_count == 4 && last_x == 50 && last_window_id == 102,
		"region changed during show was not delivered");
	change_during_show = 0;
	fb_hLinuxMaliitMethodCall(NULL, NULL, NULL, NULL, "imInitiatedHide", NULL,
		(GDBusMethodInvocation *)(uintptr_t)1, NULL);
	before = update_count;
	fb_hLinuxMaliitSetTextRegion(51, 60, 70, 80, 102);
	check(update_count == before, "dismissed context sent an update");
	change_during_show = 1;
	lose_during_show = 1;
	check(fb_hLinuxMaliitRequestKeyboard(1) == 1, "third show request failed");
	before = update_count;
	fb_hLinuxMaliitSetTextRegion(52, 60, 70, 80, 102);
	check(update_count == before, "lost context was reactivated");
	change_during_show = 0;
	lose_during_show = 0;
	area_during_show = 1;
	check(fb_hLinuxMaliitRequestKeyboard(1) == 1, "fourth show request failed");
	check(fb_hLinuxMaliitQueryKeyboard(&visible) == -1 && visible == -1,
		"show discarded an observed keyboard area");
	/* Include the actual Nemo observation, clipped panels and hostile sizes. */
	check_area(0, 533, 1280, 267, 533, 267);
	check_area(0, 780, 1280, 267, 780, 20);
	check_area(-10, -20, 1280, 50, 0, 30);
	check_area(-1280, 533, 1280, 267, 0, 0);
	check_area(1280, 533, 1280, 267, 0, 0);
	check_area(INT_MAX, 533, INT_MAX, 267, 0, 0);
	check_area(-1, 533, INT_MAX, INT_MAX, 533, 267);
	check_area(0, INT_MIN, 1280, INT_MAX, 0, 0);
	check_area(0, 800, 1280, 267, 0, 0);
	check_area(0, 533, 0, 267, 0, 0);
	check_area(0, 533, 1280, -1, 0, 0);
	check_area(0, 533, 1280, 0, 0, 0);
	check_area(0, 533, 1280, 267, 533, 267);
	int top = -7, overlap = -9;
	area_during_show = 0;
	check(fb_hLinuxMaliitRequestKeyboard(1) == 1 &&
		fb_hLinuxMaliitGetKeyboardOcclusion(1280, 800, &top, &overlap) == 1 &&
		top == 533 && overlap == 267,
		"idempotent show discarded unchanged keyboard bounds");
	area_during_show = 1;
	check(fb_hLinuxMaliitGetKeyboardOcclusion(0, 800, &top, &overlap) == 0 &&
		fb_hLinuxMaliitGetKeyboardOcclusion(1280, 0, &top, &overlap) == 0 &&
		fb_hLinuxMaliitGetKeyboardOcclusion(1280, 800, NULL, &overlap) == 0 &&
		fb_hLinuxMaliitGetKeyboardOcclusion(1280, 800, &top, NULL) == 0,
		"invalid Maliit query accepted");
	fb_hLinuxMaliitState.available = 0;
	check(fb_hLinuxMaliitGetKeyboardOcclusion(1280, 800, &top, &overlap) == 0,
		"disconnected peer retained its bounds");
	fb_hLinuxMaliitState.available = 1;
	before = update_count;
	fb_hLinuxMaliitSetWindowId(103);
	check(update_count == before + 1 && last_window_id == 103 && last_x == 52,
		"window change did not retain and publish the current region");
	fb_hLinuxMaliitSetWindowId(103);
	check(update_count == before + 1, "unchanged window sent an update");
	fb_hLinuxMaliitCancelPending();
	check(fb_hLinuxMaliitGetKeyboardOcclusion(1280, 800, &top, &overlap) == 0,
		"retired context retained its bounds");
	check(fb_hLinuxMaliitState.panel_width == 0 &&
		fb_hLinuxMaliitState.panel_height == 0,
		"retired rectangle could be revived by a later context");
	before = update_count;
	fb_hLinuxMaliitSetTextRegion(53, 60, 70, 80, 104);
	check(update_count == before, "a closed context received a later region");
	check(fb_hLinuxMaliitRequestKeyboard(1) == 1,
		"show after context close failed");
	check(update_count == before + 1 && last_x == 53 && last_window_id == 104,
		"the reopened context did not receive its current region");
	check(fb_hLinuxMaliitRequestKeyboard(0) == 1, "final hide request failed");
	check(fb_hLinuxMaliitGetKeyboardOcclusion(1280, 800, &top, &overlap) == 0,
		"hidden keyboard retained its bounds");
	area_during_show = 0;
	check(fb_hLinuxMaliitRequestKeyboard(1) == 1 &&
		fb_hLinuxMaliitGetKeyboardOcclusion(1280, 800, &top, &overlap) == 0,
		"show after hide reused the retired rectangle");
	check(fb_hLinuxMaliitRequestKeyboard(0) == 1, "cleanup hide failed");
	check(peer_references == 0, "calls retained peer references");
	puts("linux-maliit-region: passed");
	return 0;
}

/* end of linux-maliit-region.c */
