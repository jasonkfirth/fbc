/*
 * FreeBASIC Linux graphics
 * ------------------------
 *
 * File: fb_linux_maliit.c
 *
 * Purpose:
 *
 *     Connect a focused graphics process to the session Maliit input method.
 *
 * Responsibilities:
 *
 *     - load the optional GIO runtime without a build-time GLib dependency
 *     - connect to the peer endpoint published by org.maliit.server
 *     - export the input-context callbacks and request keyboard visibility
 *     - share the optional GIO binding with the KWin visibility query
 *     - return committed text and editing keys to the active graphics input
 *
 * This file intentionally does NOT contain:
 *
 *     - form-factor detection or display management
 *     - FreeBASIC widget editing policy
 *     - a compile-time dependency on Maliit or GLib headers
 */
#define _POSIX_C_SOURCE 200809L

#include "fb_linux_maliit.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FB_LINUX_MALIIT_EXTENDED_KEY(scan_code) \
	((((uint32_t)(scan_code)) << 8) | 0xffu)
/* Focus can lag behind a newly mapped window; keep retries bounded. */
#define FB_LINUX_MALIIT_FOCUS_RETRIES 32
#define FB_LINUX_MALIIT_FOCUS_RETRY_NS 250000000L
#define FB_LINUX_MALIIT_FOCUS_CALL_TIMEOUT_MS 200

/* GDBusConnectionFlags, kept here so GLib headers remain optional at build time. */
#define FB_GDBUS_AUTHENTICATION_CLIENT (1u << 0)
#define FB_GDBUS_DELAY_MESSAGE_PROCESSING (1u << 4)

typedef struct _GDBusConnection GDBusConnection;
typedef struct _GDBusNodeInfo GDBusNodeInfo;
typedef struct _GDBusInterfaceInfo GDBusInterfaceInfo;
typedef struct _GDBusMethodInvocation GDBusMethodInvocation;
typedef struct _GMainContext GMainContext;
typedef struct _GMainLoop GMainLoop;
typedef struct _GVariant GVariant;
typedef struct _GVariantBuilder GVariantBuilder;
typedef struct _GVariantType GVariantType;
typedef struct _GError GError;
typedef struct _GCancellable GCancellable;
typedef struct _GObject GObject;
typedef struct _GAsyncResult GAsyncResult;

typedef void (*FB_GASYNC_READY_CALLBACK)(GObject *, GAsyncResult *, void *);
typedef void (*FB_GCALLBACK)(void);
typedef void (*FB_GCLOSURE_NOTIFY)(void *, void *);

typedef void (*FB_GDBUS_METHOD_CALL)(GDBusConnection *, const char *,
	const char *, const char *, const char *, GVariant *,
	GDBusMethodInvocation *, void *);

typedef struct FB_GDBUS_INTERFACE_VTABLE {
	FB_GDBUS_METHOD_CALL method_call;
	void *get_property;
	void *set_property;
	void *padding[8];
} FB_GDBUS_INTERFACE_VTABLE;

typedef struct FB_LINUX_MALIIT_API {
	void *gio_library;
	void *gobject_library;
	void *glib_library;
	GDBusConnection *(*bus_get_sync)(int, void *, GError **);
	GVariant *(*connection_call_sync)(GDBusConnection *, const char *,
		const char *, const char *, const char *, GVariant *,
		const GVariantType *, int, int, void *, GError **);
	void (*connection_call)(GDBusConnection *, const char *,
		const char *, const char *, const char *, GVariant *,
		const GVariantType *, int, int, GCancellable *,
		FB_GASYNC_READY_CALLBACK, void *);
	GDBusConnection *(*connection_new_for_address_sync)(const char *, int,
		void *, void *, GError **);
	void (*connection_set_exit_on_close)(GDBusConnection *, int);
	void (*connection_start_message_processing)(GDBusConnection *);
	unsigned int (*connection_register_object)(GDBusConnection *, const char *,
		GDBusInterfaceInfo *, const FB_GDBUS_INTERFACE_VTABLE *, void *,
		void *, GError **);
	int (*connection_unregister_object)(GDBusConnection *, unsigned int);
	GDBusNodeInfo *(*node_info_new_for_xml)(const char *, GError **);
	GDBusInterfaceInfo *(*node_info_lookup_interface)(GDBusNodeInfo *,
		const char *);
	void (*node_info_unref)(GDBusNodeInfo *);
	GMainContext *(*main_context_new)(void);
	void (*main_context_push_thread_default)(GMainContext *);
	void (*main_context_pop_thread_default)(GMainContext *);
	void (*main_context_unref)(GMainContext *);
	GMainLoop *(*main_loop_new)(GMainContext *, int);
	void (*main_loop_run)(GMainLoop *);
	void (*main_loop_quit)(GMainLoop *);
	void (*main_loop_unref)(GMainLoop *);
	GVariantBuilder *(*variant_builder_new)(const GVariantType *);
	void (*variant_builder_add_value)(GVariantBuilder *, GVariant *);
	GVariant *(*variant_builder_end)(GVariantBuilder *);
	void (*variant_builder_unref)(GVariantBuilder *);
	GVariant *(*variant_new)(const char *, ...);
	GVariant *(*variant_new_string)(const char *);
	GVariant *(*variant_new_boolean)(int);
	GVariant *(*variant_new_int32)(int32_t);
	GVariant *(*variant_new_uint64)(uint64_t);
	GVariant *(*variant_new_variant)(GVariant *);
	GVariant *(*variant_new_dict_entry)(GVariant *, GVariant *);
	void (*variant_get)(GVariant *, const char *, ...);
	const char *(*variant_get_type_string)(GVariant *);
	void (*variant_unref)(GVariant *);
	void (*method_invocation_return_value)(GDBusMethodInvocation *, GVariant *);
	void *(*object_ref)(void *);
	void (*object_unref)(void *);
	unsigned long (*signal_connect_data)(void *, const char *, FB_GCALLBACK,
		void *, FB_GCLOSURE_NOTIFY, int);
	void (*error_free)(GError *);
	void (*free_memory)(void *);
} FB_LINUX_MALIIT_API;

typedef struct FB_LINUX_MALIIT_STATE {
	int initialization;
	int ready;
	int available;
	int visible;
	int panel_x;
	int panel_y;
	int panel_width;
	int panel_height;
	int context_active;
	int region_dirty;
	unsigned int context_generation;
	uint64_t keyboard_generation;
	int keyboard_requested;
	int focus_retry_pending;
	int focus_retry_running;
	int request_worker_running;
	int request_pending;
	int request_visible;
	int request_try_maliit;
	uint64_t request_generation;
	FB_LINUX_MALIIT_FALLBACK_CALLBACK request_fallback;
	int region_x;
	int region_y;
	int region_width;
	int region_height;
	uint64_t window_id;
	GDBusConnection *connection;
	FB_LINUX_MALIIT_KEY_CALLBACK key_callback;
	void *key_user_data;
} FB_LINUX_MALIIT_STATE;

static FB_LINUX_MALIIT_API fb_hLinuxMaliitApi;
static FB_LINUX_MALIIT_STATE fb_hLinuxMaliitState = {
	.visible = -2,
	.region_dirty = 1,
	.region_width = 1,
	.region_height = 1
};
/* The callback thread and graphics thread share state through this mutex. */
static pthread_mutex_t fb_hLinuxMaliitMutex = PTHREAD_MUTEX_INITIALIZER;
/*
 * A callback may use a mode's input storage until it returns. Shutdown takes
 * this gate before destroying that storage, even in non-MT rtlib builds.
 * Take the gate before the state mutex; callbacks must not call the bridge.
 */
static pthread_mutex_t fb_hLinuxMaliitCallbackMutex = PTHREAD_MUTEX_INITIALIZER;
/* Serializes peer show/hide calls and their fallback, not queue submission. */
static pthread_mutex_t fb_hLinuxMaliitRequestMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t fb_hLinuxMaliitReadyCondition = PTHREAD_COND_INITIALIZER;
static pthread_once_t fb_hLinuxMaliitApiOnce = PTHREAD_ONCE_INIT;
static int fb_hLinuxMaliitApiReady;

static const char fb_hLinuxMaliitInputContextXml[] =
	"<node>"
	"<interface name='com.meego.inputmethod.inputcontext1'>"
	"<method name='activationLostEvent'/>"
	"<method name='imInitiatedHide'/>"
	"<method name='commitString'>"
	"<arg type='s' direction='in'/><arg type='i' direction='in'/>"
	"<arg type='i' direction='in'/><arg type='i' direction='in'/>"
	"</method>"
	"<method name='keyEvent'>"
	"<arg type='i' direction='in'/><arg type='i' direction='in'/>"
	"<arg type='i' direction='in'/><arg type='s' direction='in'/>"
	"<arg type='b' direction='in'/><arg type='i' direction='in'/>"
	"<arg type='y' direction='in'/>"
	"</method>"
	"<method name='updateInputMethodArea'>"
	"<arg type='i' direction='in'/><arg type='i' direction='in'/>"
	"<arg type='i' direction='in'/><arg type='i' direction='in'/>"
	"</method>"
	"</interface>"
	"</node>";

/* POSIX permits dlsym() results to be copied into function pointers. */
#define FB_LINUX_MALIIT_LOAD_LIBRARY(library_field, soname) \
	do { \
		fb_hLinuxMaliitApi.library_field = dlopen(soname, RTLD_NOW | RTLD_LOCAL); \
		if (!fb_hLinuxMaliitApi.library_field) \
			return; \
	} while (0)

#define FB_LINUX_MALIIT_LOAD_SYMBOL(library_field, field, symbol_name) \
	do { \
		void *fb_hLinuxMaliitSymbol = dlsym( \
			fb_hLinuxMaliitApi.library_field, symbol_name); \
		if (!fb_hLinuxMaliitSymbol || \
		    sizeof(fb_hLinuxMaliitApi.field) != sizeof(fb_hLinuxMaliitSymbol)) \
			return; \
		memcpy(&fb_hLinuxMaliitApi.field, &fb_hLinuxMaliitSymbol, \
			sizeof(fb_hLinuxMaliitSymbol)); \
	} while (0)

static void fb_hLinuxMaliitLoadApi(void)
{
	FB_LINUX_MALIIT_LOAD_LIBRARY(gio_library, "libgio-2.0.so.0");
	FB_LINUX_MALIIT_LOAD_LIBRARY(gobject_library, "libgobject-2.0.so.0");
	FB_LINUX_MALIIT_LOAD_LIBRARY(glib_library, "libglib-2.0.so.0");

	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, bus_get_sync,
		"g_bus_get_sync");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, connection_call_sync,
		"g_dbus_connection_call_sync");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, connection_call,
		"g_dbus_connection_call");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, connection_new_for_address_sync,
		"g_dbus_connection_new_for_address_sync");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, connection_set_exit_on_close,
		"g_dbus_connection_set_exit_on_close");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, connection_start_message_processing,
		"g_dbus_connection_start_message_processing");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, connection_register_object,
		"g_dbus_connection_register_object");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, connection_unregister_object,
		"g_dbus_connection_unregister_object");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, node_info_new_for_xml,
		"g_dbus_node_info_new_for_xml");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, node_info_lookup_interface,
		"g_dbus_node_info_lookup_interface");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, node_info_unref,
		"g_dbus_node_info_unref");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, main_context_new,
		"g_main_context_new");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, main_context_push_thread_default,
		"g_main_context_push_thread_default");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, main_context_pop_thread_default,
		"g_main_context_pop_thread_default");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, main_context_unref,
		"g_main_context_unref");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, main_loop_new,
		"g_main_loop_new");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, main_loop_run,
		"g_main_loop_run");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, main_loop_quit,
		"g_main_loop_quit");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, main_loop_unref,
		"g_main_loop_unref");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_builder_new,
		"g_variant_builder_new");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_builder_add_value,
		"g_variant_builder_add_value");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_builder_end,
		"g_variant_builder_end");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_builder_unref,
		"g_variant_builder_unref");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_new,
		"g_variant_new");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_new_string,
		"g_variant_new_string");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_new_boolean,
		"g_variant_new_boolean");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_new_int32,
		"g_variant_new_int32");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_new_uint64,
		"g_variant_new_uint64");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_new_variant,
		"g_variant_new_variant");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_new_dict_entry,
		"g_variant_new_dict_entry");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_get,
		"g_variant_get");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_get_type_string,
		"g_variant_get_type_string");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, variant_unref,
		"g_variant_unref");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gio_library, method_invocation_return_value,
		"g_dbus_method_invocation_return_value");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gobject_library, object_ref,
		"g_object_ref");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gobject_library, object_unref,
		"g_object_unref");
	FB_LINUX_MALIIT_LOAD_SYMBOL(gobject_library, signal_connect_data,
		"g_signal_connect_data");
	FB_LINUX_MALIIT_LOAD_SYMBOL(glib_library, error_free, "g_error_free");
	FB_LINUX_MALIIT_LOAD_SYMBOL(glib_library, free_memory, "g_free");
	fb_hLinuxMaliitApiReady = 1;
}

#undef FB_LINUX_MALIIT_LOAD_LIBRARY
#undef FB_LINUX_MALIIT_LOAD_SYMBOL

static void fb_hLinuxMaliitAddStateValue(GVariantBuilder *builder,
	const char *key, GVariant *value)
{
	GVariant *entry = fb_hLinuxMaliitApi.variant_new_dict_entry(
		fb_hLinuxMaliitApi.variant_new_string(key),
		fb_hLinuxMaliitApi.variant_new_variant(value));
	fb_hLinuxMaliitApi.variant_builder_add_value(builder, entry);
}

/* The caller holds fb_hLinuxMaliitMutex while taking this state snapshot. */
static GVariant *fb_hLinuxMaliitBuildWidgetStateLocked(void)
{
	static const char dictionary_type[] = "a{sv}";
	GVariantBuilder *builder;
	GVariant *dictionary;

	builder = fb_hLinuxMaliitApi.variant_builder_new(
		(const GVariantType *)dictionary_type);
	if (!builder)
		return NULL;
	fb_hLinuxMaliitAddStateValue(builder, "focusState",
		fb_hLinuxMaliitApi.variant_new_boolean(1));
	/* The legacy key queue cannot represent or replace uncommitted preedit. */
	fb_hLinuxMaliitAddStateValue(builder, "predictionEnabled",
		fb_hLinuxMaliitApi.variant_new_boolean(0));
	fb_hLinuxMaliitAddStateValue(builder, "surroundingText",
		fb_hLinuxMaliitApi.variant_new_string(""));
	fb_hLinuxMaliitAddStateValue(builder, "cursorPosition",
		fb_hLinuxMaliitApi.variant_new_int32(0));
	fb_hLinuxMaliitAddStateValue(builder, "anchorPosition",
		fb_hLinuxMaliitApi.variant_new_int32(0));
	fb_hLinuxMaliitAddStateValue(builder, "cursorRectangle",
		fb_hLinuxMaliitApi.variant_new("(iiii)",
			fb_hLinuxMaliitState.region_x, fb_hLinuxMaliitState.region_y,
			fb_hLinuxMaliitState.region_width,
			fb_hLinuxMaliitState.region_height));
	fb_hLinuxMaliitAddStateValue(builder, "winId",
		fb_hLinuxMaliitApi.variant_new_uint64(fb_hLinuxMaliitState.window_id));
	dictionary = fb_hLinuxMaliitApi.variant_builder_end(builder);
	fb_hLinuxMaliitApi.variant_builder_unref(builder);
	return dictionary;
}

static void fb_hLinuxMaliitPostUtf8(const char *text, size_t length)
{
	FB_LINUX_MALIIT_KEY_CALLBACK callback;
	void *user_data;
	size_t offset = 0;
	size_t posted = 0;

	if (!text || !length || length > 1024)
		return;
	pthread_mutex_lock(&fb_hLinuxMaliitCallbackMutex);
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	callback = fb_hLinuxMaliitState.key_callback;
	user_data = fb_hLinuxMaliitState.key_user_data;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (!callback) {
		pthread_mutex_unlock(&fb_hLinuxMaliitCallbackMutex);
		return;
	}

	while ((offset < length) && (posted < 256)) {
		const unsigned char *bytes = (const unsigned char *)text + offset;
		uint32_t codepoint;
		size_t count;
		if (bytes[0] < 0x80u) {
			codepoint = bytes[0];
			count = 1;
		} else if ((bytes[0] >= 0xc2u) && (bytes[0] <= 0xdfu) &&
			(offset + 1 < length) && ((bytes[1] & 0xc0u) == 0x80u)) {
			codepoint = ((uint32_t)(bytes[0] & 0x1fu) << 6) |
				(uint32_t)(bytes[1] & 0x3fu);
			count = 2;
		} else if ((bytes[0] >= 0xe0u) && (bytes[0] <= 0xefu) &&
			(offset + 2 < length) && ((bytes[1] & 0xc0u) == 0x80u) &&
			((bytes[2] & 0xc0u) == 0x80u) &&
			((bytes[0] != 0xe0u) || (bytes[1] >= 0xa0u)) &&
			((bytes[0] != 0xedu) || (bytes[1] <= 0x9fu))) {
			codepoint = ((uint32_t)(bytes[0] & 0x0fu) << 12) |
				((uint32_t)(bytes[1] & 0x3fu) << 6) |
				(uint32_t)(bytes[2] & 0x3fu);
			count = 3;
		} else if ((bytes[0] >= 0xf0u) && (bytes[0] <= 0xf4u) &&
			(offset + 3 < length) && ((bytes[1] & 0xc0u) == 0x80u) &&
			((bytes[2] & 0xc0u) == 0x80u) &&
			((bytes[3] & 0xc0u) == 0x80u) &&
			((bytes[0] != 0xf0u) || (bytes[1] >= 0x90u)) &&
			((bytes[0] != 0xf4u) || (bytes[1] <= 0x8fu))) {
			codepoint = ((uint32_t)(bytes[0] & 0x07u) << 18) |
				((uint32_t)(bytes[1] & 0x3fu) << 12) |
				((uint32_t)(bytes[2] & 0x3fu) << 6) |
				(uint32_t)(bytes[3] & 0x3fu);
			count = 4;
		} else {
			offset++;
			continue;
		}
		if (codepoint != 0) {
			callback(user_data, codepoint, FB_LINUX_MALIIT_TEXT);
			posted++;
		}
		offset += count;
	}
	pthread_mutex_unlock(&fb_hLinuxMaliitCallbackMutex);
}

static size_t fb_hLinuxMaliitStringLength(const char *text, size_t limit)
{
	size_t length = 0;
	if (!text)
		return 0;
	while ((length < limit) && text[length])
		length++;
	return length;
}

static void fb_hLinuxMaliitPostKey(uint32_t key)
{
	FB_LINUX_MALIIT_KEY_CALLBACK callback;
	void *user_data;
	pthread_mutex_lock(&fb_hLinuxMaliitCallbackMutex);
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	callback = fb_hLinuxMaliitState.key_callback;
	user_data = fb_hLinuxMaliitState.key_user_data;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (callback && key)
		callback(user_data, key, FB_LINUX_MALIIT_KEY);
	pthread_mutex_unlock(&fb_hLinuxMaliitCallbackMutex);
}

static uint32_t fb_hLinuxMaliitMapKey(int qt_key)
{
	switch ((uint32_t)qt_key) {
	case 0x01000000u: return 27u;
	case 0x01000001u: return '\t';
	case 0x01000003u: return 8u;
	case 0x01000004u:
	case 0x01000005u: return 13u;
	case 0x01000007u: return FB_LINUX_MALIIT_EXTENDED_KEY('S');
	case 0x01000010u: return FB_LINUX_MALIIT_EXTENDED_KEY('G');
	case 0x01000011u: return FB_LINUX_MALIIT_EXTENDED_KEY('O');
	case 0x01000012u: return FB_LINUX_MALIIT_EXTENDED_KEY('K');
	case 0x01000013u: return FB_LINUX_MALIIT_EXTENDED_KEY('H');
	case 0x01000014u: return FB_LINUX_MALIIT_EXTENDED_KEY('M');
	case 0x01000015u: return FB_LINUX_MALIIT_EXTENDED_KEY('P');
	case 0x01000016u: return FB_LINUX_MALIIT_EXTENDED_KEY('I');
	case 0x01000017u: return FB_LINUX_MALIIT_EXTENDED_KEY('Q');
	default: return 0;
	}
}

static unsigned char fb_hLinuxMaliitAsciiToLower(unsigned char value)
{
	if ((value >= 'A') && (value <= 'Z'))
		return (unsigned char)(value + ('a' - 'A'));
	return value;
}

static int fb_hLinuxMaliitIsLomiri(const char *desktop)
{
	const char *cursor;
	if (!desktop)
		return 0;
	for (cursor = desktop; *cursor; cursor++) {
		if (strlen(cursor) >= 6 &&
		    (fb_hLinuxMaliitAsciiToLower((unsigned char)cursor[0]) == 'l') &&
		    (fb_hLinuxMaliitAsciiToLower((unsigned char)cursor[1]) == 'o') &&
		    (fb_hLinuxMaliitAsciiToLower((unsigned char)cursor[2]) == 'm') &&
		    (fb_hLinuxMaliitAsciiToLower((unsigned char)cursor[3]) == 'i') &&
		    (fb_hLinuxMaliitAsciiToLower((unsigned char)cursor[4]) == 'r') &&
		    (fb_hLinuxMaliitAsciiToLower((unsigned char)cursor[5]) == 'i'))
			return 1;
	}
	return 0;
}

static void fb_hLinuxMaliitMethodCall(GDBusConnection *connection,
	const char *sender, const char *object_path, const char *interface_name,
	const char *method_name, GVariant *parameters,
	GDBusMethodInvocation *invocation, void *user_data)
{
	(void)connection;
	(void)sender;
	(void)object_path;
	(void)interface_name;
	(void)user_data;

	if (strcmp(method_name, "commitString") == 0) {
		const char *text = NULL;
		int replace_start = 0, replace_length = 0, cursor_position = -1;
		if (fb_hLinuxMaliitApi.variant_get) {
			fb_hLinuxMaliitApi.variant_get(parameters, "(&siii)", &text,
				&replace_start, &replace_length, &cursor_position);
			(void)replace_start;
			(void)replace_length;
			(void)cursor_position;
			if (text)
				fb_hLinuxMaliitPostUtf8(text,
					fb_hLinuxMaliitStringLength(text, 1025));
		}
	} else if (strcmp(method_name, "keyEvent") == 0) {
		int type = 0, key = 0, modifiers = 0, auto_repeat = 0, count = 0;
		const char *text = NULL;
		unsigned char request_type = 0;
		if (fb_hLinuxMaliitApi.variant_get) {
			fb_hLinuxMaliitApi.variant_get(parameters, "(iii&sbiy)", &type,
				&key, &modifiers, &text, &auto_repeat, &count,
				&request_type);
			(void)modifiers;
			(void)auto_repeat;
			(void)count;
			if ((type == 6) && (request_type != 1)) {
				if (text && *text)
					fb_hLinuxMaliitPostUtf8(text,
						fb_hLinuxMaliitStringLength(text, 1025));
				else {
					uint32_t mapped_key = fb_hLinuxMaliitMapKey(key);
					if (mapped_key)
						fb_hLinuxMaliitPostKey(mapped_key);
				}
			}
		}
	} else if (strcmp(method_name, "updateInputMethodArea") == 0) {
		int x = 0, y = 0, width = 0, height = 0;
		fb_hLinuxMaliitApi.variant_get(parameters, "(iiii)", &x, &y,
			&width, &height);
		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		fb_hLinuxMaliitState.panel_x = x;
		fb_hLinuxMaliitState.panel_y = y;
		fb_hLinuxMaliitState.panel_width = width;
		fb_hLinuxMaliitState.panel_height = height;
		fb_hLinuxMaliitState.visible = (height > 0) ? -1 : 0;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	} else if ((strcmp(method_name, "activationLostEvent") == 0) ||
		(strcmp(method_name, "imInitiatedHide") == 0)) {
		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		fb_hLinuxMaliitState.visible = 0;
		fb_hLinuxMaliitState.context_active = 0;
		fb_hLinuxMaliitState.panel_width = 0;
		fb_hLinuxMaliitState.panel_height = 0;
		fb_hLinuxMaliitState.region_dirty = 1;
		fb_hLinuxMaliitState.context_generation++;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	}

	fb_hLinuxMaliitApi.method_invocation_return_value(invocation,
		fb_hLinuxMaliitApi.variant_new("()"));
}

/*
 * GDBus delivers "closed" in the thread-default context where the connection
 * was created. The worker creates it in its own context so an input method
 * restart can retire the old peer even when the application has no GLib loop.
 */
static void fb_hLinuxMaliitConnectionClosed(GDBusConnection *connection,
	int remote_peer_vanished, GError *error, void *user_data)
{
	GMainLoop *loop = user_data;
	(void)remote_peer_vanished;
	(void)error;
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	if (fb_hLinuxMaliitState.connection == connection) {
		fb_hLinuxMaliitState.connection = NULL;
		fb_hLinuxMaliitState.initialization = -1;
		fb_hLinuxMaliitState.available = 0;
		fb_hLinuxMaliitState.visible = -2;
		fb_hLinuxMaliitState.context_active = 0;
		fb_hLinuxMaliitState.panel_width = 0;
		fb_hLinuxMaliitState.panel_height = 0;
		fb_hLinuxMaliitState.region_dirty = 1;
		fb_hLinuxMaliitState.context_generation++;
		pthread_cond_broadcast(&fb_hLinuxMaliitReadyCondition);
	}
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitApi.main_loop_quit(loop);
}

static void *fb_hLinuxMaliitMainLoop(void *user_data)
{
	static const FB_GDBUS_INTERFACE_VTABLE vtable = {
		fb_hLinuxMaliitMethodCall, NULL, NULL, { NULL }
	};
	GMainContext *context;
	GMainLoop *loop = NULL;
	GDBusNodeInfo *node_info;
	GDBusInterfaceInfo *interface_info;
	GError *error = NULL;
	unsigned int registration = 0;
	GDBusConnection *connection = NULL;
	char *address = user_data;

	context = fb_hLinuxMaliitApi.main_context_new();
	if (!context)
		goto failed;
	fb_hLinuxMaliitApi.main_context_push_thread_default(context);
	/*
	 * Maliit can send a method as soon as authentication finishes. Queue it
	 * until GDBus has finished initialization and our object is registered;
	 * otherwise older GDBus versions can try to reply to an unready connection.
	 */
	connection = fb_hLinuxMaliitApi.connection_new_for_address_sync(address,
		FB_GDBUS_AUTHENTICATION_CLIENT | FB_GDBUS_DELAY_MESSAGE_PROCESSING,
		NULL, NULL, &error);
	free(address);
	address = NULL;
	if (!connection)
		goto failed_with_context;
	fb_hLinuxMaliitApi.connection_set_exit_on_close(connection, 0);
	node_info = fb_hLinuxMaliitApi.node_info_new_for_xml(
		fb_hLinuxMaliitInputContextXml, &error);
	if (!node_info)
		goto failed_with_context;
	interface_info = fb_hLinuxMaliitApi.node_info_lookup_interface(node_info,
		"com.meego.inputmethod.inputcontext1");
	registration = interface_info ?
		fb_hLinuxMaliitApi.connection_register_object(connection,
			"/com/meego/inputmethod/inputcontext", interface_info,
			&vtable, NULL, NULL, &error) : 0;
	fb_hLinuxMaliitApi.node_info_unref(node_info);
	if (!registration)
		goto failed_with_context;
	fb_hLinuxMaliitApi.connection_start_message_processing(connection);
	loop = fb_hLinuxMaliitApi.main_loop_new(context, 0);
	if (!loop)
		goto failed_with_context;
	if (!fb_hLinuxMaliitApi.signal_connect_data(connection, "closed",
		(FB_GCALLBACK)fb_hLinuxMaliitConnectionClosed, loop, NULL, 0))
		goto failed_with_context;
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitState.connection = connection;
	fb_hLinuxMaliitState.ready = 1;
	fb_hLinuxMaliitState.available = 1;
	fb_hLinuxMaliitState.initialization = 2;
	pthread_cond_broadcast(&fb_hLinuxMaliitReadyCondition);
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitApi.main_loop_run(loop);
	fb_hLinuxMaliitApi.main_loop_unref(loop);
	(void)fb_hLinuxMaliitApi.connection_unregister_object(connection, registration);
	fb_hLinuxMaliitApi.object_unref(connection);
	fb_hLinuxMaliitApi.main_context_pop_thread_default(context);
	fb_hLinuxMaliitApi.main_context_unref(context);
	return NULL;

failed_with_context:
	if (error)
		fb_hLinuxMaliitApi.error_free(error);
	if (loop)
		fb_hLinuxMaliitApi.main_loop_unref(loop);
	if (registration)
		(void)fb_hLinuxMaliitApi.connection_unregister_object(connection, registration);
	if (connection)
		fb_hLinuxMaliitApi.object_unref(connection);
	fb_hLinuxMaliitApi.main_context_pop_thread_default(context);
	fb_hLinuxMaliitApi.main_context_unref(context);
failed:
	free(address);
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitState.ready = 1;
	fb_hLinuxMaliitState.available = 0;
	fb_hLinuxMaliitState.initialization = -1;
	pthread_cond_broadcast(&fb_hLinuxMaliitReadyCondition);
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	return NULL;
}

static int fb_hLinuxMaliitEnsureConnection(void)
{
	GDBusConnection *session_connection;
	GVariant *reply = NULL, *address_variant = NULL;
	GVariant *address_request = NULL;
	GError *error = NULL;
	const char *address = NULL;
	char peer_address[4096];
	char *worker_address;
	pthread_attr_t attributes;
	pthread_t thread;
	int initialize = 0;
	int thread_started = 0;

	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	if (fb_hLinuxMaliitState.initialization == 2) {
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
		return 1;
	}
	if (fb_hLinuxMaliitState.initialization == 1) {
		while (!fb_hLinuxMaliitState.ready)
			pthread_cond_wait(&fb_hLinuxMaliitReadyCondition,
				&fb_hLinuxMaliitMutex);
		initialize = fb_hLinuxMaliitState.available;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
		return initialize;
	}
	fb_hLinuxMaliitState.initialization = 1;
	fb_hLinuxMaliitState.ready = 0;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);

	pthread_once(&fb_hLinuxMaliitApiOnce, fb_hLinuxMaliitLoadApi);
	if (!fb_hLinuxMaliitApiReady)
		goto failed;
	session_connection = fb_hLinuxMaliitApi.bus_get_sync(2, NULL, &error);
	if (!session_connection)
		goto failed;
	address_request = fb_hLinuxMaliitApi.variant_new("(ss)",
		"org.maliit.Server.Address", "address");
	if (!address_request) {
		fb_hLinuxMaliitApi.object_unref(session_connection);
		goto failed;
	}
	reply = fb_hLinuxMaliitApi.connection_call_sync(session_connection,
		"org.maliit.server", "/org/maliit/server/address",
		"org.freedesktop.DBus.Properties", "Get", address_request,
		NULL, 0, 1000, NULL,
		&error);
	/* call_sync consumes the floating parameter returned by variant_new(). */
	fb_hLinuxMaliitApi.object_unref(session_connection);
	if (!reply)
		goto failed;
	fb_hLinuxMaliitApi.variant_get(reply, "(v)", &address_variant);
	fb_hLinuxMaliitApi.variant_unref(reply);
	if (!address_variant)
		goto failed;
	fb_hLinuxMaliitApi.variant_get(address_variant, "&s", &address);
	if (!address || !*address ||
	    fb_hLinuxMaliitStringLength(address, sizeof(peer_address)) >=
		sizeof(peer_address)) {
		fb_hLinuxMaliitApi.variant_unref(address_variant);
		goto failed;
	}
	memcpy(peer_address, address, strlen(address) + 1);
	fb_hLinuxMaliitApi.variant_unref(address_variant);
	worker_address = strdup(peer_address);
	if (!worker_address)
		goto failed;
	if (pthread_attr_init(&attributes) == 0) {
		if (pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED) == 0 &&
		    pthread_create(&thread, &attributes, fb_hLinuxMaliitMainLoop,
			worker_address) == 0)
			thread_started = 1;
		pthread_attr_destroy(&attributes);
	}
	if (!thread_started) {
		free(worker_address);
		goto failed;
	}
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	while (!fb_hLinuxMaliitState.ready)
		pthread_cond_wait(&fb_hLinuxMaliitReadyCondition,
			&fb_hLinuxMaliitMutex);
	initialize = fb_hLinuxMaliitState.available;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (initialize)
		return 1;

failed:
	if (error)
		fb_hLinuxMaliitApi.error_free(error);
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitState.ready = 1;
	fb_hLinuxMaliitState.available = 0;
	if (fb_hLinuxMaliitState.initialization != 2)
		fb_hLinuxMaliitState.initialization = -1;
	pthread_cond_broadcast(&fb_hLinuxMaliitReadyCondition);
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	return 0;
}

static GVariant *fb_hLinuxMaliitCall(const char *method, GVariant *parameters)
{
	GDBusConnection *connection;
	GError *error = NULL;
	GVariant *reply;

	/* The worker may retire a closed peer while this synchronous call waits. */
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	connection = fb_hLinuxMaliitState.connection;
	if (connection)
		fb_hLinuxMaliitApi.object_ref(connection);
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (!connection || !parameters) {
		if (connection)
			fb_hLinuxMaliitApi.object_unref(connection);
		if (parameters)
			fb_hLinuxMaliitApi.variant_unref(parameters);
		return NULL;
	}
	reply = fb_hLinuxMaliitApi.connection_call_sync(connection, NULL,
		"/com/meego/inputmethod/uiserver1",
		"com.meego.inputmethod.uiserver1", method, parameters,
		NULL, 0, 1000, NULL, &error);
	fb_hLinuxMaliitApi.object_unref(connection);
	/* call_sync consumes floating parameters; do not unref them a second time. */
	if (error)
		fb_hLinuxMaliitApi.error_free(error);
	return reply;
}

void fb_hLinuxMaliitSetKeyCallback(FB_LINUX_MALIIT_KEY_CALLBACK callback,
	void *user_data)
{
	int unchanged;
	/*
	 * A repeated request can arrive while the caller holds a driver lock and
	 * an input callback is waiting for that lock. It does not need the callback
	 * gate when it leaves the registration unchanged.
	 */
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	unchanged = fb_hLinuxMaliitState.key_callback == callback &&
		fb_hLinuxMaliitState.key_user_data == user_data;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (unchanged)
		return;
	pthread_mutex_lock(&fb_hLinuxMaliitCallbackMutex);
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitState.key_callback = callback;
	fb_hLinuxMaliitState.key_user_data = user_data;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	pthread_mutex_unlock(&fb_hLinuxMaliitCallbackMutex);
}

/*
 * Region updates can follow focus on the very next GUI frame. Queue them
 * without waiting for Maliit, and hold the state lock until the message is
 * queued so a concurrent hide cannot be followed by a stale focused update.
 * GDBusConnection accepts calls from any thread; a NULL callback sends this
 * update without requesting a reply.
 */
static void fb_hLinuxMaliitSendRegionLocked(void)
{
	GVariant *widget_state;
	GVariant *parameters;

	if (!fb_hLinuxMaliitState.context_active ||
	    !fb_hLinuxMaliitState.region_dirty ||
	    !fb_hLinuxMaliitState.connection)
		return;
	widget_state = fb_hLinuxMaliitBuildWidgetStateLocked();
	if (!widget_state)
		return;
	parameters = fb_hLinuxMaliitApi.variant_new("(@a{sv}b)", widget_state, 1);
	if (!parameters) {
		fb_hLinuxMaliitApi.variant_unref(widget_state);
		return;
	}
	fb_hLinuxMaliitApi.connection_call(fb_hLinuxMaliitState.connection, NULL,
		"/com/meego/inputmethod/uiserver1",
		"com.meego.inputmethod.uiserver1", "updateWidgetInformation",
		parameters, NULL, 0, 1000, NULL, NULL, NULL);
	/* The asynchronous call consumes the floating parameter. */
	fb_hLinuxMaliitState.region_dirty = 0;
}

void fb_hLinuxMaliitSetWindowId(uint64_t window_id)
{
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	if (fb_hLinuxMaliitState.window_id != window_id) {
		fb_hLinuxMaliitState.window_id = window_id;
		fb_hLinuxMaliitState.region_dirty = 1;
	}
	fb_hLinuxMaliitSendRegionLocked();
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
}

void fb_hLinuxMaliitSetTextRegion(int x, int y, int width, int height,
	uint64_t window_id)
{
	if (x < 0 || y < 0 || width < 1 || height < 1)
		return;
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	if (fb_hLinuxMaliitState.region_x != x ||
	    fb_hLinuxMaliitState.region_y != y ||
	    fb_hLinuxMaliitState.region_width != width ||
	    fb_hLinuxMaliitState.region_height != height ||
	    fb_hLinuxMaliitState.window_id != window_id) {
		fb_hLinuxMaliitState.region_x = x;
		fb_hLinuxMaliitState.region_y = y;
		fb_hLinuxMaliitState.region_width = width;
		fb_hLinuxMaliitState.region_height = height;
		fb_hLinuxMaliitState.window_id = window_id;
		fb_hLinuxMaliitState.region_dirty = 1;
	}
	fb_hLinuxMaliitSendRegionLocked();
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
}

/* The caller serializes show and hide requests with the request mutex. */
static int fb_hLinuxMaliitSendKeyboardLocked(int visible)
{
	GVariant *reply;
	GVariant *widget_state;
	GVariant *parameters;
	unsigned int context_generation = 0;
	int result = 0;
	if (!fb_hLinuxMaliitEnsureConnection())
		return 0;
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitState.context_active = 0;
	fb_hLinuxMaliitState.region_dirty = 1;
	context_generation = fb_hLinuxMaliitState.context_generation;
	if (!visible) {
		fb_hLinuxMaliitState.panel_width = 0;
		fb_hLinuxMaliitState.panel_height = 0;
	}
	if (visible)
		fb_hLinuxMaliitState.visible = -2;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (visible) {
		reply = fb_hLinuxMaliitCall("activateContext",
			fb_hLinuxMaliitApi.variant_new("()"));
		if (!reply)
			goto done;
		fb_hLinuxMaliitApi.variant_unref(reply);
		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		widget_state = fb_hLinuxMaliitBuildWidgetStateLocked();
		if (widget_state)
			fb_hLinuxMaliitState.region_dirty = 0;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
		if (!widget_state)
			goto done;
		parameters = fb_hLinuxMaliitApi.variant_new("(@a{sv}b)",
			widget_state, 1);
		if (!parameters) {
			fb_hLinuxMaliitApi.variant_unref(widget_state);
			goto done;
		}
		reply = fb_hLinuxMaliitCall("updateWidgetInformation", parameters);
		if (!reply)
			goto done;
		fb_hLinuxMaliitApi.variant_unref(reply);
		reply = fb_hLinuxMaliitCall("showInputMethod",
			fb_hLinuxMaliitApi.variant_new("()"));
	} else {
		reply = fb_hLinuxMaliitCall("hideInputMethod",
			fb_hLinuxMaliitApi.variant_new("()"));
	}
	if (!reply)
		goto done;
	fb_hLinuxMaliitApi.variant_unref(reply);
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	if (visible) {
		/* Do not revive a context that Maliit dismissed during the request. */
		if (context_generation == fb_hLinuxMaliitState.context_generation) {
			fb_hLinuxMaliitState.context_active = 1;
			fb_hLinuxMaliitSendRegionLocked();
		}
	} else {
		fb_hLinuxMaliitState.visible = 0;
	}
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	result = 1;
done:
	return result;
}

/*
 * Maliit asks Lomiri whether the requesting process has focus before it
 * activates a context. A newly mapped Wayland surface can receive its first
 * request before the shell has finished assigning focus. Return -1 when the
 * optional focus interface is unavailable and let Maliit make its own decision.
 */
static int fb_hLinuxMaliitFocusState(void)
{
	static const struct {
		const char *name;
		const char *path;
	} interfaces[] = {
		{ "com.lomiri.Shell.FocusInfo", "/com/lomiri/Shell/FocusInfo" },
		{ "com.canonical.Unity.FocusInfo", "/com/canonical/Unity/FocusInfo" }
	};
	GDBusConnection *connection;
	GVariant *reply;
	GError *error = NULL;
	pid_t pid = getpid();
	int focused = -1;
	size_t i;

	if (pid <= 0 || (uint64_t)pid > UINT32_MAX)
		return -1;
	connection = fb_hLinuxMaliitApi.bus_get_sync(2, NULL, &error);
	if (!connection) {
		if (error)
			fb_hLinuxMaliitApi.error_free(error);
		return -1;
	}
	for (i = 0; i < sizeof(interfaces) / sizeof(interfaces[0]); i++) {
		GVariant *parameters = fb_hLinuxMaliitApi.variant_new("(u)", (uint32_t)pid);
		if (!parameters)
			break;
		reply = fb_hLinuxMaliitApi.connection_call_sync(connection,
			interfaces[i].name, interfaces[i].path, interfaces[i].name,
			"isPidFocused", parameters, NULL, 0,
			FB_LINUX_MALIIT_FOCUS_CALL_TIMEOUT_MS, NULL, &error);
		if (reply) {
			if (strcmp(fb_hLinuxMaliitApi.variant_get_type_string(reply), "(b)") == 0)
				fb_hLinuxMaliitApi.variant_get(reply, "(b)", &focused);
			fb_hLinuxMaliitApi.variant_unref(reply);
			if (focused >= 0)
				break;
		}
		if (error) {
			fb_hLinuxMaliitApi.error_free(error);
			error = NULL;
		}
	}
	if (error)
		fb_hLinuxMaliitApi.error_free(error);
	fb_hLinuxMaliitApi.object_unref(connection);
	return focused;
}

/*
 * One detached worker handles the latest pending show request. The request
 * mutex orders its eventual show with explicit hides, and the generation
 * prevents a cancelled request from opening the keyboard on a later window.
 */
static void *fb_hLinuxMaliitFocusRetry(void *unused)
{
	const struct timespec delay = { 0, FB_LINUX_MALIIT_FOCUS_RETRY_NS };
	uint64_t generation = 0;
	int attempts = 0;
	(void)unused;

	for (;;) {
		int focus, send = 0, stop = 0;
		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		if (!fb_hLinuxMaliitState.keyboard_requested ||
		    !fb_hLinuxMaliitState.focus_retry_pending) {
			fb_hLinuxMaliitState.focus_retry_running = 0;
			pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
			return NULL;
		}
		if (generation != fb_hLinuxMaliitState.keyboard_generation) {
			generation = fb_hLinuxMaliitState.keyboard_generation;
			attempts = 0;
		}
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);

		focus = fb_hLinuxMaliitFocusState();
		if (focus != 0) {
			pthread_mutex_lock(&fb_hLinuxMaliitRequestMutex);
			pthread_mutex_lock(&fb_hLinuxMaliitMutex);
			if (!fb_hLinuxMaliitState.keyboard_requested ||
			    !fb_hLinuxMaliitState.focus_retry_pending) {
				fb_hLinuxMaliitState.focus_retry_running = 0;
				stop = 1;
			} else if (generation == fb_hLinuxMaliitState.keyboard_generation) {
				fb_hLinuxMaliitState.focus_retry_pending = 0;
				fb_hLinuxMaliitState.focus_retry_running = 0;
				send = 1;
			}
			pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
			if (send) {
				(void)fb_hLinuxMaliitSendKeyboardLocked(1);
				pthread_mutex_lock(&fb_hLinuxMaliitMutex);
				stop = !fb_hLinuxMaliitState.keyboard_requested &&
					generation != fb_hLinuxMaliitState.keyboard_generation;
				pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
				if (stop)
					(void)fb_hLinuxMaliitSendKeyboardLocked(0);
			}
			pthread_mutex_unlock(&fb_hLinuxMaliitRequestMutex);
			if (send || stop)
				return NULL;
			continue;
		}
		attempts++;
		if (attempts >= FB_LINUX_MALIIT_FOCUS_RETRIES) {
			pthread_mutex_lock(&fb_hLinuxMaliitMutex);
			if (generation == fb_hLinuxMaliitState.keyboard_generation) {
				fb_hLinuxMaliitState.focus_retry_pending = 0;
				fb_hLinuxMaliitState.focus_retry_running = 0;
				pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
				return NULL;
			}
			pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
		}
		(void)nanosleep(&delay, NULL);
	}
}

static int fb_hLinuxMaliitQueueFocusRetry(void)
{
	pthread_attr_t attributes;
	pthread_t thread;
	int started = 0;

	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	if (fb_hLinuxMaliitState.focus_retry_running) {
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
		return 1;
	}
	fb_hLinuxMaliitState.focus_retry_running = 1;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (pthread_attr_init(&attributes) == 0) {
		if (pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED) == 0 &&
		    pthread_create(&thread, &attributes, fb_hLinuxMaliitFocusRetry, NULL) == 0)
			started = 1;
		pthread_attr_destroy(&attributes);
	}
	if (!started) {
		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		fb_hLinuxMaliitState.focus_retry_running = 0;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	}
	return started;
}

void fb_hLinuxMaliitCancelPending(void)
{
	pthread_mutex_lock(&fb_hLinuxMaliitCallbackMutex);
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitState.keyboard_requested = 0;
	fb_hLinuxMaliitState.focus_retry_pending = 0;
	fb_hLinuxMaliitState.request_pending = 0;
	/* Do not publish a later window's region to the closing input context. */
	fb_hLinuxMaliitState.context_active = 0;
	fb_hLinuxMaliitState.panel_width = 0;
	fb_hLinuxMaliitState.panel_height = 0;
	fb_hLinuxMaliitState.context_generation++;
	fb_hLinuxMaliitState.region_dirty = 1;
	fb_hLinuxMaliitState.keyboard_generation++;
	fb_hLinuxMaliitState.key_callback = NULL;
	fb_hLinuxMaliitState.key_user_data = NULL;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	pthread_mutex_unlock(&fb_hLinuxMaliitCallbackMutex);
}

static int fb_hLinuxMaliitSessionSupported(void)
{
	const char *desktop = getenv("XDG_CURRENT_DESKTOP");
	const char *mir_socket = getenv("MIR_SOCKET");
	const char *input_method = getenv("QT_IM_MODULE");

	/* Sailfish identifies its session through Qt's Maliit input method. */
	return (mir_socket && *mir_socket) || fb_hLinuxMaliitIsLomiri(desktop) ||
		(input_method && strcmp(input_method, "Maliit") == 0);
}

/* fb_hLinuxMaliitMutex must be held when checking a reserved request. */
static int fb_hLinuxMaliitRequestCurrent(uint64_t generation, int visible)
{
	return generation == fb_hLinuxMaliitState.keyboard_generation &&
		fb_hLinuxMaliitState.keyboard_requested == (visible != 0);
}

/*
 * The generation is reserved before entering this function. A close or a
 * newer request can therefore invalidate queued work even while another
 * request is waiting on the peer. The request mutex orders actual IPC only.
 */
static int fb_hLinuxMaliitProcessKeyboard(int visible, uint64_t generation)
{
	const char *wayland_display = getenv("WAYLAND_DISPLAY");
	int result = 0;
	int cancelled;

	pthread_mutex_lock(&fb_hLinuxMaliitRequestMutex);
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	cancelled = !fb_hLinuxMaliitRequestCurrent(generation, visible);
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (cancelled) {
		result = 1;
		goto done;
	}
	if (visible && wayland_display && *wayland_display &&
	    fb_hLinuxMaliitEnsureConnection() && fb_hLinuxMaliitFocusState() == 0) {
		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		cancelled = !fb_hLinuxMaliitRequestCurrent(generation, visible);
		if (!cancelled)
			fb_hLinuxMaliitState.focus_retry_pending = 1;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
		if (cancelled) {
			result = 1;
			goto done;
		}
		if (fb_hLinuxMaliitQueueFocusRetry()) {
			result = 1;
			goto done;
		}
		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		if (fb_hLinuxMaliitRequestCurrent(generation, visible))
			fb_hLinuxMaliitState.focus_retry_pending = 0;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	}
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	cancelled = !fb_hLinuxMaliitRequestCurrent(generation, visible);
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (cancelled) {
		result = 1;
		goto done;
	}
	result = fb_hLinuxMaliitSendKeyboardLocked(visible);
	if (visible) {
		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		cancelled = !fb_hLinuxMaliitState.keyboard_requested &&
			generation != fb_hLinuxMaliitState.keyboard_generation;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
		/* A timed-out show may still have reached the input-method server. */
		if (cancelled)
			(void)fb_hLinuxMaliitSendKeyboardLocked(0);
	}
done:
	pthread_mutex_unlock(&fb_hLinuxMaliitRequestMutex);
	return result;
}

int fb_hLinuxMaliitRequestKeyboard(int visible)
{
	uint64_t generation;

	if (!fb_hLinuxMaliitSessionSupported())
		return 0;
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitState.keyboard_requested = visible != 0;
	fb_hLinuxMaliitState.focus_retry_pending = 0;
	fb_hLinuxMaliitState.request_pending = 0;
	generation = ++fb_hLinuxMaliitState.keyboard_generation;
	if (visible)
		fb_hLinuxMaliitState.visible = -2;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	return fb_hLinuxMaliitProcessKeyboard(visible, generation);
}

/*
 * Run the fallback in the same order as Maliit requests. It has no mode
 * pointer and completes before returning. If a close happens during a show,
 * issue the matching hide after the in-flight request has finished.
 */
static void fb_hLinuxMaliitRequestFallback(int visible, uint64_t generation,
	FB_LINUX_MALIIT_FALLBACK_CALLBACK fallback)
{
	int current, cancelled;

	if (!fallback)
		return;
	pthread_mutex_lock(&fb_hLinuxMaliitRequestMutex);
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	current = fb_hLinuxMaliitRequestCurrent(generation, visible);
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	if (current) {
		fallback(visible);
		if (visible) {
			pthread_mutex_lock(&fb_hLinuxMaliitMutex);
			cancelled = !fb_hLinuxMaliitState.keyboard_requested &&
				generation != fb_hLinuxMaliitState.keyboard_generation;
			pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
			if (cancelled)
				fallback(0);
		}
	}
	pthread_mutex_unlock(&fb_hLinuxMaliitRequestMutex);
}

/* One worker coalesces Maliit and fallback requests without a mode pointer. */
static void *fb_hLinuxMaliitRequestWorker(void *unused)
{
	(void)unused;
	for (;;) {
		FB_LINUX_MALIIT_FALLBACK_CALLBACK fallback;
		uint64_t generation;
		int visible, try_maliit;

		pthread_mutex_lock(&fb_hLinuxMaliitMutex);
		if (!fb_hLinuxMaliitState.request_pending) {
			fb_hLinuxMaliitState.request_worker_running = 0;
			pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
			return NULL;
		}
		generation = fb_hLinuxMaliitState.request_generation;
		visible = fb_hLinuxMaliitState.request_visible;
		try_maliit = fb_hLinuxMaliitState.request_try_maliit;
		fallback = fb_hLinuxMaliitState.request_fallback;
		fb_hLinuxMaliitState.request_pending = 0;
		pthread_mutex_unlock(&fb_hLinuxMaliitMutex);

		if (!try_maliit ||
		    !fb_hLinuxMaliitProcessKeyboard(visible, generation))
			fb_hLinuxMaliitRequestFallback(visible, generation, fallback);
	}
}

int fb_hLinuxMaliitQueueKeyboard(int visible,
	FB_LINUX_MALIIT_FALLBACK_CALLBACK fallback)
{
	pthread_attr_t attributes;
	pthread_t thread;
	int started = 1;
	int try_maliit = fb_hLinuxMaliitSessionSupported();
	const char *session_bus = getenv("DBUS_SESSION_BUS_ADDRESS");

	if (!try_maliit && (!fallback || !session_bus || !*session_bus))
		return 0;
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	fb_hLinuxMaliitState.keyboard_requested = visible != 0;
	fb_hLinuxMaliitState.focus_retry_pending = 0;
	fb_hLinuxMaliitState.request_generation =
		++fb_hLinuxMaliitState.keyboard_generation;
	fb_hLinuxMaliitState.request_visible = visible != 0;
	fb_hLinuxMaliitState.request_try_maliit = try_maliit;
	fb_hLinuxMaliitState.request_fallback = fallback;
	fb_hLinuxMaliitState.request_pending = 1;
	if (visible)
		fb_hLinuxMaliitState.visible = -2;
	if (!fb_hLinuxMaliitState.request_worker_running) {
		started = 0;
		fb_hLinuxMaliitState.request_worker_running = 1;
		if (pthread_attr_init(&attributes) == 0) {
			if (pthread_attr_setdetachstate(&attributes,
			    PTHREAD_CREATE_DETACHED) == 0 &&
			    pthread_create(&thread, &attributes,
			    fb_hLinuxMaliitRequestWorker, NULL) == 0)
				started = 1;
			(void)pthread_attr_destroy(&attributes);
		}
		if (!started) {
			fb_hLinuxMaliitState.request_worker_running = 0;
			fb_hLinuxMaliitState.request_pending = 0;
			fb_hLinuxMaliitState.keyboard_requested = 0;
			fb_hLinuxMaliitState.keyboard_generation++;
		}
	}
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	return started;
}

int fb_hLinuxMaliitQueryKeyboard(int *visible)
{
	int available;
	if (!visible)
		return 0;
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	available = fb_hLinuxMaliitState.available;
	*visible = fb_hLinuxMaliitState.visible;
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	return available ? -1 : 0;
}

int fb_hLinuxMaliitGetKeyboardOcclusion(int width, int height,
	int *top, int *overlap)
{
	int result = 0;
	int panel_top;
	int64_t bottom;
	if (!top || !overlap || width <= 0 || height <= 0)
		return 0;
	/*
	 * Read the last peer observation without making a bus call on a graphics
	 * frame. A hidden, disconnected or retired context cannot reuse its old
	 * rectangle. A repeated show can temporarily mark visibility unknown;
	 * Maliit need not resend an unchanged area, so retain that observation
	 * until hide, dismissal, disconnect or cancellation explicitly retires it.
	 * Use wide arithmetic before clipping untrusted peer sizes.
	 */
	pthread_mutex_lock(&fb_hLinuxMaliitMutex);
	if (fb_hLinuxMaliitState.available &&
		fb_hLinuxMaliitState.context_active &&
		fb_hLinuxMaliitState.visible != 0 &&
		fb_hLinuxMaliitState.panel_width > 0 &&
		fb_hLinuxMaliitState.panel_height > 0 &&
		fb_hLinuxMaliitState.panel_x < width &&
		(int64_t)fb_hLinuxMaliitState.panel_x +
			fb_hLinuxMaliitState.panel_width > 0 &&
		fb_hLinuxMaliitState.panel_y < height) {
		panel_top = fb_hLinuxMaliitState.panel_y;
		if (panel_top < 0)
			panel_top = 0;
		bottom = (int64_t)fb_hLinuxMaliitState.panel_y +
			fb_hLinuxMaliitState.panel_height;
		if (bottom > height)
			bottom = height;
		if (bottom > panel_top) {
			*top = panel_top;
			*overlap = (int)(bottom - panel_top);
			result = 1;
		}
	}
	pthread_mutex_unlock(&fb_hLinuxMaliitMutex);
	return result;
}

/*
 * KWin exposes visibility on its session bus, independently of the keyboard
 * implementation. This is called only by the OSK query worker because even a
 * bounded D-Bus call must not hold up a graphics frame. The optional GIO loader
 * is shared with the Maliit bridge to avoid another library dependency.
 */
int fb_hLinuxKwinQueryKeyboard(int *visible)
{
	/* GIO values are kept local because the binding has no GIO headers. */
	enum {
		FB_KWIN_SESSION_BUS = 2,
		FB_KWIN_NO_AUTO_START = 1,
		FB_KWIN_QUERY_TIMEOUT_MS = 1000
	};
	GDBusConnection *connection;
	GVariant *parameters, *reply, *value = NULL;
	GError *error = NULL;
	int state = 0, available = 0;

	if (!visible)
		return 0;
	*visible = -2;
	pthread_once(&fb_hLinuxMaliitApiOnce, fb_hLinuxMaliitLoadApi);
	if (!fb_hLinuxMaliitApiReady)
		return 0;
	connection = fb_hLinuxMaliitApi.bus_get_sync(FB_KWIN_SESSION_BUS, NULL, &error);
	if (!connection)
		goto done;
	parameters = fb_hLinuxMaliitApi.variant_new("(ss)",
		"org.kde.kwin.VirtualKeyboard", "visible");
	if (!parameters) {
		fb_hLinuxMaliitApi.object_unref(connection);
		goto done;
	}
	/* Do not start a desktop service merely to inspect its keyboard state. */
	reply = fb_hLinuxMaliitApi.connection_call_sync(connection,
		"org.kde.KWin", "/VirtualKeyboard",
		"org.freedesktop.DBus.Properties", "Get", parameters,
		NULL, FB_KWIN_NO_AUTO_START, FB_KWIN_QUERY_TIMEOUT_MS, NULL, &error);
	fb_hLinuxMaliitApi.object_unref(connection);
	if (!reply)
		goto done;
	if (strcmp(fb_hLinuxMaliitApi.variant_get_type_string(reply), "(v)") == 0)
		fb_hLinuxMaliitApi.variant_get(reply, "(v)", &value);
	fb_hLinuxMaliitApi.variant_unref(reply);
	if (value) {
		if (strcmp(fb_hLinuxMaliitApi.variant_get_type_string(value), "b") == 0) {
			fb_hLinuxMaliitApi.variant_get(value, "b", &state);
			*visible = state ? -1 : 0;
			available = -1;
		}
		fb_hLinuxMaliitApi.variant_unref(value);
	}
done:
	if (error)
		fb_hLinuxMaliitApi.error_free(error);
	return available;
}

/* end of fb_linux_maliit.c */
