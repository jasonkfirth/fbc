/*
 * Project: FreeBASIC Unix runtime: sys_hnotify.c
 * Purpose: Present Linux and BSD notifications through native desktop services.
 * Responsibilities: Dynamic dependencies, owner-thread contexts, plain text,
 * and Luna-service webOS toasts.
 * This file intentionally does NOT contain: shell execution, service lifecycle,
 * or X11/Wayland code.
 * File: drivers/unix/sys_hnotify.c
 */
/* pipe2 atomically prevents descriptor inheritance by concurrent execs. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../../fb_services.h"

#if defined(HOST_LINUX) || defined(HOST_FREEBSD) || defined(HOST_NETBSD) || \
    defined(HOST_OPENBSD) || defined(HOST_DRAGONFLY)
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <spawn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

/* Use libnotify/GLib's actual opaque tags in dynamic function signatures.
 * Their object layouts stay private; calling through a differently typed
 * function pointer would not be a valid C interface even on a matching ABI.
 */
typedef struct _NotifyNotification NotifyNotification;
typedef struct _GDBusConnection GDBusConnection;
typedef struct _GVariant GVariant;
typedef struct _GVariantType GVariantType;
typedef struct _GError GError;

/* Stable GLib GList layout returned by notify_get_server_caps. Each data
 * string and the list itself are owned by the caller of that API.
 */
typedef struct _GList {
	void *data;
	struct _GList *next;
	struct _GList *prev;
} GList;

/* Bound the synchronous D-Bus round trip so a vanished notification service
 * cannot hold the application's graphics thread indefinitely. */
#define FB_NOTIFY_DBUS_TIMEOUT_MSEC 1000
/* The server chooses its normal lifetime when expiry is negative. */
#define FB_NOTIFY_DBUS_DEFAULT_EXPIRE_MSEC (-1)

/* LuneOS exposes notifications through Luna Service 2 rather than the
 * freedesktop interface. Its installed luna-send client avoids the incompatible
 * legacy LS2 client protocol and keeps the UI call bounded to three seconds.
 */
#ifndef FB_NOTIFY_LUNA_SEND_PATH
#define FB_NOTIFY_LUNA_SEND_PATH "/usr/bin/luna-send"
#endif
#define FB_NOTIFY_LUNA_CREATE_URI "luna://com.webos.notification/createToast"
#define FB_NOTIFY_LUNA_CLOSE_URI "luna://com.webos.notification/closeToast"
#define FB_NOTIFY_LUNA_WAIT_MSEC "3000"
/* luna-send replies with a small JSON object; bound output if its behavior changes. */
#define FB_NOTIFY_LUNA_RESPONSE_MAX 4096u
/* Keep server-generated toast identifiers bounded inside each context. */
#define FB_NOTIFY_LUNA_TOAST_ID_MAX 255u

typedef struct FB_NOTIFY_UNIX {
	pthread_t owner;
	NotifyNotification *notification;
	GDBusConnection *connection;
	uint32_t notification_id;
	int dbus_backend;
	int luna_backend;
	int library_reference;
	char app_name[FB_NOTIFY_APP_MAX + 1u];
	char app_id[FB_NOTIFY_APP_MAX + 1u];
	char toast_id[FB_NOTIFY_LUNA_TOAST_ID_MAX + 1u];
} FB_NOTIFY_UNIX;

/* ------------------------------------------------------------------------- */
/* LuneOS payload encoding and service client                                 */
/* ------------------------------------------------------------------------- */

static int luna_runtime_available(void)
{
	const char *service_name = getenv("LS2_NAME");
	return service_name != NULL && service_name[0] != '\0' &&
		access(FB_NOTIFY_LUNA_SEND_PATH, X_OK) == 0;
}

static void set_notification_app_id(FB_NOTIFY_UNIX *context)
{
	const char *app_id = getenv("APP_ID");
	size_t length;
	if (app_id != NULL) {
		length = strnlen(app_id, FB_NOTIFY_APP_MAX + 1u);
		if (length > 0 && length <= FB_NOTIFY_APP_MAX &&
			fb_hServiceUtf8(app_id, length)) {
			memcpy(context->app_id, app_id, length + 1u);
			return;
		}
	}
	memcpy(context->app_id, context->app_name, strlen(context->app_name) + 1u);
}

static int add_size(size_t *total, size_t amount)
{
	if (*total > SIZE_MAX - amount)
		return FALSE;
	*total += amount;
	return TRUE;
}

static int html_escaped_size(const char *text, size_t *output_size)
{
	size_t size = 0;
	const unsigned char *cursor = (const unsigned char *)text;
	while (*cursor != '\0') {
		size_t amount = 1;
		switch (*cursor) {
		case '&': amount = 5; break;
		case '"': amount = 6; break;
		case '\'': amount = 5; break;
		case '<':
		case '>': amount = 4; break;
		}
		if (!add_size(&size, amount))
			return FALSE;
		++cursor;
	}
	*output_size = size;
	return TRUE;
}

static int json_escaped_size(const char *text, size_t *output_size)
{
	size_t size = 0;
	const unsigned char *cursor = (const unsigned char *)text;
	while (*cursor != '\0') {
		size_t amount = (*cursor < 0x20) ? 6u :
			((*cursor == '"' || *cursor == '\\') ? 2u : 1u);
		if (!add_size(&size, amount))
			return FALSE;
		++cursor;
	}
	*output_size = size;
	return TRUE;
}

static int append_bytes(char *buffer, size_t capacity, size_t *position,
	const char *bytes, size_t length)
{
	if (*position > capacity || length > capacity - *position)
		return FALSE;
	memcpy(buffer + *position, bytes, length);
	*position += length;
	return TRUE;
}

static int append_html_text(char *buffer, size_t capacity, size_t *position,
	const char *text)
{
	const unsigned char *cursor = (const unsigned char *)text;
	while (*cursor != '\0') {
		const char *replacement = NULL;
		size_t length = 1;
		switch (*cursor) {
		case '&': replacement = "&amp;"; length = 5; break;
		case '"': replacement = "&quot;"; length = 6; break;
		case '\'': replacement = "&#39;"; length = 5; break;
		case '<': replacement = "&lt;"; length = 4; break;
		case '>': replacement = "&gt;"; length = 4; break;
		}
		if (replacement != NULL) {
			if (!append_bytes(buffer, capacity, position, replacement, length))
				return FALSE;
		} else {
			char value = (char)*cursor;
			if (!append_bytes(buffer, capacity, position, &value, 1u))
				return FALSE;
		}
		++cursor;
	}
	return TRUE;
}

static int append_json_text(char *buffer, size_t capacity, size_t *position,
	const char *text)
{
	static const char hexadecimal[] = "0123456789abcdef";
	const unsigned char *cursor = (const unsigned char *)text;
	while (*cursor != '\0') {
		char escaped[6];
		const char *replacement = NULL;
		size_t length = 1;
		switch (*cursor) {
		case '"': replacement = "\\\""; length = 2; break;
		case '\\': replacement = "\\\\"; length = 2; break;
		case '\b': replacement = "\\b"; length = 2; break;
		case '\f': replacement = "\\f"; length = 2; break;
		case '\n': replacement = "\\n"; length = 2; break;
		case '\r': replacement = "\\r"; length = 2; break;
		case '\t': replacement = "\\t"; length = 2; break;
		default:
			if (*cursor < 0x20) {
				escaped[0] = '\\';
				escaped[1] = 'u';
				escaped[2] = '0';
				escaped[3] = '0';
				escaped[4] = hexadecimal[*cursor >> 4];
				escaped[5] = hexadecimal[*cursor & 0x0f];
				replacement = escaped;
				length = sizeof(escaped);
			}
			break;
		}
		if (replacement != NULL) {
			if (!append_bytes(buffer, capacity, position, replacement, length))
				return FALSE;
		} else {
			char value = (char)*cursor;
			if (!append_bytes(buffer, capacity, position, &value, 1u))
				return FALSE;
		}
		++cursor;
	}
	return TRUE;
}

static char *make_luna_toast_payload(FB_NOTIFY_UNIX *context,
	const char *title, const char *body)
{
	static const char title_prefix[] = "<h3>";
	static const char title_suffix[] = "</h3>";
	static const char prefix[] = "{\"sourceId\":\"";
	static const char middle[] = "\",\"message\":\"";
	static const char suffix[] = "\"}";
	size_t title_size, body_size, message_size = 0;
	size_t app_json_size, message_json_size, payload_size = 0;
	size_t position = 0;
	char *message, *payload;
	if (!html_escaped_size(title, &title_size) ||
		!html_escaped_size(body, &body_size) ||
		!add_size(&message_size, sizeof(title_prefix) - 1u) ||
		!add_size(&message_size, sizeof(title_suffix) - 1u) ||
		!add_size(&message_size, title_size) ||
		!add_size(&message_size, body_size) || message_size == SIZE_MAX ||
		!json_escaped_size(context->app_id, &app_json_size))
		return NULL;
	message = malloc(message_size + 1u);
	if (message == NULL)
		return NULL;
	position = 0;
	if (!append_bytes(message, message_size, &position, title_prefix,
		sizeof(title_prefix) - 1u) ||
		!append_html_text(message, message_size, &position, title) ||
		!append_bytes(message, message_size, &position, title_suffix,
			sizeof(title_suffix) - 1u) ||
		!append_html_text(message, message_size, &position, body)) {
		free(message);
		return NULL;
	}
	message[position] = '\0';
	if (!json_escaped_size(message, &message_json_size) ||
		!add_size(&payload_size, sizeof(prefix) - 1u) ||
		!add_size(&payload_size, app_json_size) ||
		!add_size(&payload_size, sizeof(middle) - 1u) ||
		!add_size(&payload_size, message_json_size) ||
		!add_size(&payload_size, sizeof(suffix) - 1u) ||
		payload_size == SIZE_MAX) {
		free(message);
		return NULL;
	}
	payload = malloc(payload_size + 1u);
	if (payload == NULL) {
		free(message);
		return NULL;
	}
	position = 0;
	if (!append_bytes(payload, payload_size, &position, prefix, sizeof(prefix) - 1u) ||
		!append_json_text(payload, payload_size, &position, context->app_id) ||
		!append_bytes(payload, payload_size, &position, middle, sizeof(middle) - 1u) ||
		!append_json_text(payload, payload_size, &position, message) ||
		!append_bytes(payload, payload_size, &position, suffix, sizeof(suffix) - 1u)) {
		free(message);
		free(payload);
		return NULL;
	}
	payload[position] = '\0';
	free(message);
	return payload;
}

static char *make_luna_close_payload(const char *toast_id)
{
	static const char prefix[] = "{\"toastId\":\"";
	static const char suffix[] = "\"}";
	size_t id_json_size, payload_size = 0, position = 0;
	char *payload;
	if (!json_escaped_size(toast_id, &id_json_size) ||
		!add_size(&payload_size, sizeof(prefix) - 1u) ||
		!add_size(&payload_size, id_json_size) ||
		!add_size(&payload_size, sizeof(suffix) - 1u) || payload_size == SIZE_MAX)
		return NULL;
	payload = malloc(payload_size + 1u);
	if (payload == NULL)
		return NULL;
	if (!append_bytes(payload, payload_size, &position, prefix, sizeof(prefix) - 1u) ||
		!append_json_text(payload, payload_size, &position, toast_id) ||
		!append_bytes(payload, payload_size, &position, suffix, sizeof(suffix) - 1u)) {
		free(payload);
		return NULL;
	}
	payload[position] = '\0';
	return payload;
}

static int json_field_is_true(const char *json, const char *field)
{
	char key[64];
	const char *value;
	int length = snprintf(key, sizeof(key), "\"%s\"", field);
	if (length < 0 || (size_t)length >= sizeof(key))
		return FALSE;
	value = strstr(json, key);
	if (value == NULL || (value = strchr(value + length, ':')) == NULL)
		return FALSE;
	++value;
	while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n')
		++value;
	return strncmp(value, "true", 4u) == 0 &&
		(value[4] == ',' || value[4] == '}' || value[4] == ' ' ||
		 value[4] == '\t' || value[4] == '\r' || value[4] == '\n');
}

static int json_copy_string_field(const char *json, const char *field,
	char *output, size_t output_size)
{
	char key[64];
	const char *value;
	size_t used = 0;
	int length = snprintf(key, sizeof(key), "\"%s\"", field);
	if (length < 0 || (size_t)length >= sizeof(key) || output_size == 0)
		return FALSE;
	value = strstr(json, key);
	if (value == NULL || (value = strchr(value + length, ':')) == NULL)
		return FALSE;
	++value;
	while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n')
		++value;
	if (*value++ != '"')
		return FALSE;
	while (*value != '\0' && *value != '"') {
		if (*value == '\\' || (unsigned char)*value < 0x20 || used + 1u >= output_size)
			return FALSE;
		output[used++] = *value++;
	}
	if (*value != '"' || used == 0)
		return FALSE;
	output[used] = '\0';
	return TRUE;
}

/* Keep original pipe descriptors above the standard streams. A managed
 * application may start with any of 0, 1 or 2 closed; spawn's later close
 * actions must not close a stream just installed by dup2. */
static int create_luna_pipe(int descriptors[2])
{
#ifdef HOST_LINUX
	if (pipe2(descriptors, O_CLOEXEC) != 0)
		return FALSE;
#else
	if (pipe(descriptors) != 0)
		return FALSE;
	if (fcntl(descriptors[0], F_SETFD, FD_CLOEXEC) < 0 ||
		fcntl(descriptors[1], F_SETFD, FD_CLOEXEC) < 0) {
		close(descriptors[0]);
		close(descriptors[1]);
		return FALSE;
	}
#endif
	for (int index = 0; index < 2; ++index) {
		if (descriptors[index] > STDERR_FILENO)
			continue;
		int replacement = fcntl(descriptors[index], F_DUPFD_CLOEXEC,
			STDERR_FILENO + 1);
		if (replacement < 0) {
			close(descriptors[0]);
			close(descriptors[1]);
			return FALSE;
		}
		close(descriptors[index]);
		descriptors[index] = replacement;
	}
	return TRUE;
}

static int run_luna_send(FB_NOTIFY_UNIX *context, const char *method,
	const char *payload, char *response, size_t response_size)
{
	int input_pipe[2], output_pipe[2];
	int status, actions_initialized = FALSE, output_failed = FALSE;
	int output_too_large = FALSE;
	pid_t child, waited;
	ssize_t bytes_read;
	size_t used = 0;
	char bytes[256];
	posix_spawn_file_actions_t actions;
	char *arguments[] = {
		FB_NOTIFY_LUNA_SEND_PATH, "-n", "1", "-w",
		FB_NOTIFY_LUNA_WAIT_MSEC, "-a", context->app_id,
		(char *)method, (char *)payload, NULL
	};
	int spawn_result;
	if (response == NULL || response_size == 0 || !create_luna_pipe(output_pipe))
		return FB_SERVICE_FAILED;
	if (!create_luna_pipe(input_pipe)) {
		close(output_pipe[0]);
		close(output_pipe[1]);
		return FB_SERVICE_FAILED;
	}
	spawn_result = posix_spawn_file_actions_init(&actions);
	if (spawn_result == 0) {
		actions_initialized = TRUE;
		spawn_result = posix_spawn_file_actions_adddup2(&actions,
			input_pipe[0], STDIN_FILENO);
	}
	if (spawn_result == 0) {
		spawn_result = posix_spawn_file_actions_adddup2(&actions,
			output_pipe[1], STDOUT_FILENO);
	}
	if (spawn_result == 0)
		spawn_result = posix_spawn_file_actions_addclose(&actions, output_pipe[0]);
	if (spawn_result == 0)
		spawn_result = posix_spawn_file_actions_addclose(&actions, output_pipe[1]);
	if (spawn_result == 0)
		spawn_result = posix_spawn_file_actions_addclose(&actions, input_pipe[0]);
	if (spawn_result == 0)
		spawn_result = posix_spawn_file_actions_addclose(&actions, input_pipe[1]);
	if (spawn_result == 0)
		spawn_result = posix_spawn_file_actions_addopen(&actions, STDERR_FILENO,
			"/dev/null", O_WRONLY, 0);
	if (spawn_result == 0)
		spawn_result = posix_spawn(&child, FB_NOTIFY_LUNA_SEND_PATH, &actions,
			NULL, arguments, environ);
	if (actions_initialized)
		posix_spawn_file_actions_destroy(&actions);
	close(output_pipe[1]);
	close(input_pipe[0]);
	if (spawn_result != 0) {
		close(output_pipe[0]);
		close(input_pipe[1]);
		return FB_SERVICE_UNAVAILABLE;
	}
	/* luna-send -n watches stdin for HUP while awaiting its asynchronous
	 * reply. Retain this writer until the child exits. Inherited EOF or
	 * /dev/null makes it quit early; caller input must remain untouched. */
	for (;;) {
		bytes_read = read(output_pipe[0], bytes, sizeof(bytes));
		if (bytes_read == 0)
			break;
		if (bytes_read < 0) {
			if (errno == EINTR)
				continue;
			output_failed = TRUE;
			break;
		}
		if ((size_t)bytes_read >= response_size - used) {
			output_too_large = TRUE;
		} else if (!output_too_large) {
			memcpy(response + used, bytes, (size_t)bytes_read);
			used += (size_t)bytes_read;
		}
	}
	close(output_pipe[0]);
	do {
		waited = waitpid(child, &status, 0);
	} while (waited < 0 && errno == EINTR);
	close(input_pipe[1]);
	if (output_failed || output_too_large || waited < 0 ||
		!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		return FB_SERVICE_UNAVAILABLE;
	response[used] = '\0';
	return FB_SERVICE_OK;
}

/* ------------------------------------------------------------------------- */
/* Dynamic notification libraries and backend selection                     */
/* ------------------------------------------------------------------------- */

/* libnotify has process-global initialization. Serialize our references and
 * keep its module loaded while any context exists. An application mixing
 * direct libnotify use must initialize it first and outlive these contexts.
 */
static pthread_mutex_t library_mutex = PTHREAD_MUTEX_INITIALIZER;
static void *library;
static unsigned int references;
static int own_init;
static int dbus_backend;
static void *gio_library;
static void *glib_library;
static void *gobject_library;
static int (*notify_init_fn)(const char *);
static int (*notify_is_initted_fn)(void);
static void (*notify_uninit_fn)(void);
static NotifyNotification *(*notification_new)(const char *, const char *, const char *);
static int (*notification_update)(NotifyNotification *, const char *, const char *, const char *);
static void (*notification_app_name)(NotifyNotification *, const char *);
static void (*notification_hint_string)(NotifyNotification *, const char *, const char *);
static int (*notification_show)(NotifyNotification *, GError **);
static int (*notification_close)(NotifyNotification *, GError **);
static char *(*markup_escape)(const char *, ssize_t);
static void (*object_unref)(void *);
static void (*memory_free)(void *);
static GList *(*server_capabilities)(void);
static void (*list_free)(GList *);
static GDBusConnection *(*bus_get_sync_fn)(int, void *, GError **);
static GVariant *(*connection_call_sync_fn)(GDBusConnection *, const char *,
	const char *, const char *, const char *, GVariant *, const void *, int,
	int, void *, GError **);
static GVariant *(*variant_new_fn)(const char *, ...);
static GVariant *(*variant_new_strv_fn)(const char *const *, ssize_t);
static GVariant *(*variant_new_array_fn)(const GVariantType *, GVariant *const *, size_t);
static void (*variant_get_fn)(GVariant *, const char *, ...);
static void (*variant_unref_fn)(GVariant *);
static void (*error_free_fn)(GError *);
static void (*gobject_unref_fn)(void *);

static int load_library(void)
{
	void *address;
#define LOAD_FUNCTION(field, symbol) \
	do { \
		address = dlsym(library, symbol); \
		if (address == NULL) return FALSE; \
		_Static_assert(sizeof(field) == sizeof(address), "POSIX dlsym ABI"); \
		memcpy(&field, &address, sizeof(field)); \
	} while (0)
	library = dlopen("libnotify.so.4", RTLD_NOW | RTLD_LOCAL);
	if (library == NULL)
		library = dlopen("libnotify.so", RTLD_NOW | RTLD_LOCAL);
	if (library == NULL)
		return FALSE;
	LOAD_FUNCTION(notify_init_fn, "notify_init");
	LOAD_FUNCTION(notify_is_initted_fn, "notify_is_initted");
	LOAD_FUNCTION(notify_uninit_fn, "notify_uninit");
	LOAD_FUNCTION(notification_new, "notify_notification_new");
	LOAD_FUNCTION(notification_update, "notify_notification_update");
	LOAD_FUNCTION(notification_app_name, "notify_notification_set_app_name");
	LOAD_FUNCTION(notification_hint_string, "notify_notification_set_hint_string");
	LOAD_FUNCTION(notification_show, "notify_notification_show");
	LOAD_FUNCTION(notification_close, "notify_notification_close");
	LOAD_FUNCTION(markup_escape, "g_markup_escape_text");
	LOAD_FUNCTION(object_unref, "g_object_unref");
	LOAD_FUNCTION(memory_free, "g_free");
	LOAD_FUNCTION(server_capabilities, "notify_get_server_caps");
	LOAD_FUNCTION(list_free, "g_list_free");
#undef LOAD_FUNCTION
	return TRUE;
}

/*
 * Sailfish supplies the freedesktop notification interface on its session
 * bus, but does not ship libnotify. Keep the primary desktop path above and
 * resolve only the small GIO surface needed for the standard D-Bus methods.
 */
static void unload_gdbus_libraries(void)
{
	if (gobject_library != NULL)
		dlclose(gobject_library);
	if (glib_library != NULL)
		dlclose(glib_library);
	if (gio_library != NULL)
		dlclose(gio_library);
	gobject_library = NULL;
	glib_library = NULL;
	gio_library = NULL;
	bus_get_sync_fn = NULL;
	connection_call_sync_fn = NULL;
	variant_new_fn = NULL;
	variant_new_strv_fn = NULL;
	variant_new_array_fn = NULL;
	variant_get_fn = NULL;
	variant_unref_fn = NULL;
	error_free_fn = NULL;
	gobject_unref_fn = NULL;
}

static int load_gdbus_libraries(void)
{
	void *address;
#define LOAD_GDBUS_FUNCTION(module, field, symbol) \
	do { \
		address = dlsym((module), (symbol)); \
		if (address == NULL) goto failed; \
		_Static_assert(sizeof(field) == sizeof(address), "POSIX dlsym ABI"); \
		memcpy(&(field), &address, sizeof(field)); \
	} while (0)
	gio_library = dlopen("libgio-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
	glib_library = dlopen("libglib-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
	gobject_library = dlopen("libgobject-2.0.so.0", RTLD_NOW | RTLD_LOCAL);
	if (gio_library == NULL || glib_library == NULL || gobject_library == NULL)
		goto failed;
	LOAD_GDBUS_FUNCTION(gio_library, bus_get_sync_fn, "g_bus_get_sync");
	LOAD_GDBUS_FUNCTION(gio_library, connection_call_sync_fn,
		"g_dbus_connection_call_sync");
	LOAD_GDBUS_FUNCTION(glib_library, variant_new_fn, "g_variant_new");
	LOAD_GDBUS_FUNCTION(glib_library, variant_new_strv_fn, "g_variant_new_strv");
	LOAD_GDBUS_FUNCTION(glib_library, variant_new_array_fn,
		"g_variant_new_array");
	LOAD_GDBUS_FUNCTION(glib_library, variant_get_fn, "g_variant_get");
	LOAD_GDBUS_FUNCTION(glib_library, variant_unref_fn, "g_variant_unref");
	LOAD_GDBUS_FUNCTION(glib_library, error_free_fn, "g_error_free");
	LOAD_GDBUS_FUNCTION(gobject_library, gobject_unref_fn, "g_object_unref");
#undef LOAD_GDBUS_FUNCTION
	return TRUE;

failed:
#undef LOAD_GDBUS_FUNCTION
	unload_gdbus_libraries();
	return FALSE;
}

/* library_mutex is held by the caller while releasing the last context. */
static void unload_notification_library_locked(void)
{
	if (dbus_backend) {
		unload_gdbus_libraries();
		dbus_backend = FALSE;
	} else if (library != NULL) {
		if (own_init)
			notify_uninit_fn();
		dlclose(library);
		library = NULL;
	}
	own_init = FALSE;
}

/* library_mutex must be held. */
static void release_notification_reference_locked(void)
{
	if (references == 0)
		return;
	--references;
	if (references == 0)
		unload_notification_library_locked();
}

int fb_hNotifyCreate(const char *app_name, void **output)
{
	FB_NOTIFY_UNIX *context = calloc(1, sizeof(*context));
	GError *error = NULL;
	GVariant *reply = NULL;
	int ready = TRUE;
	if (context == NULL)
		return FB_SERVICE_FAILED;
	context->owner = pthread_self();
	memcpy(context->app_name, app_name, strlen(app_name) + 1u);
	set_notification_app_id(context);
	pthread_mutex_lock(&library_mutex);
	if (references == 0) {
		ready = load_library();
		if (ready) {
			own_init = !notify_is_initted_fn();
			if (own_init)
				ready = notify_init_fn(app_name);
		}
		if (ready) {
			dbus_backend = FALSE;
		} else {
			if (library != NULL) {
				dlclose(library);
				library = NULL;
			}
			own_init = FALSE;
			ready = load_gdbus_libraries();
			dbus_backend = ready;
		}
	}
	if (ready && references != UINT_MAX) {
		++references;
		context->library_reference = TRUE;
	} else {
		ready = FALSE;
	}
	if (!ready && luna_runtime_available()) {
		context->luna_backend = TRUE;
		ready = TRUE;
	}
	context->dbus_backend = context->library_reference ? dbus_backend : FALSE;
	pthread_mutex_unlock(&library_mutex);
	if (!ready) {
		free(context);
		return FB_SERVICE_UNAVAILABLE;
	}
	if (context->luna_backend) {
		*output = context;
		return FB_SERVICE_OK;
	}
	if (context->dbus_backend) {
		context->connection = bus_get_sync_fn(2, NULL, &error);
		if (error != NULL) {
			error_free_fn(error);
			error = NULL;
		}
		if (context->connection != NULL) {
			reply = connection_call_sync_fn(context->connection,
				"org.freedesktop.Notifications",
				"/org/freedesktop/Notifications",
				"org.freedesktop.Notifications", "GetServerInformation",
				NULL, NULL, 0, FB_NOTIFY_DBUS_TIMEOUT_MSEC, NULL, &error);
			if (error != NULL)
				error_free_fn(error);
			if (reply != NULL)
				variant_unref_fn(reply);
		}
		if (reply == NULL) {
			if (context->connection != NULL)
				gobject_unref_fn(context->connection);
			context->connection = NULL;
			pthread_mutex_lock(&library_mutex);
			release_notification_reference_locked();
			context->library_reference = FALSE;
			pthread_mutex_unlock(&library_mutex);
			if (luna_runtime_available()) {
				context->dbus_backend = FALSE;
				context->luna_backend = TRUE;
				*output = context;
				return FB_SERVICE_OK;
			}
			free(context);
			return FB_SERVICE_UNAVAILABLE;
		}
	}
	*output = context;
	return FB_SERVICE_OK;
}

static GVariant *call_notification_server(FB_NOTIFY_UNIX *context,
	const char *method, GVariant *parameters, GError **error)
{
	return connection_call_sync_fn(context->connection,
		"org.freedesktop.Notifications",
		"/org/freedesktop/Notifications",
		"org.freedesktop.Notifications", method, parameters, NULL, 0,
		FB_NOTIFY_DBUS_TIMEOUT_MSEC, NULL, error);
}

/* APP_ID names the installed desktop entry without its .desktop suffix.
 * Mobile shells can accept Notify yet hide an unidentified application. Both
 * freedesktop backends publish the identity captured at context creation.
 * GVariant containers consume their children's floating references. */
static GVariant *make_dbus_notification_hints(const FB_NOTIFY_UNIX *context)
{
	GVariant *value = variant_new_fn("s", context->app_id);
	GVariant *entry, *hints;
	if (value == NULL)
		return NULL;
	entry = variant_new_fn("{sv}", "desktop-entry", value);
	if (entry == NULL) {
		variant_unref_fn(value);
		return NULL;
	}
	hints = variant_new_array_fn((const GVariantType *)"{sv}", &entry, 1);
	if (hints == NULL)
		variant_unref_fn(entry);
	return hints;
}

static int send_dbus_notification(FB_NOTIFY_UNIX *context,
	const char *title, const char *body)
{
	const char *empty_actions[1] = { NULL };
	GVariant *actions, *hints, *parameters, *reply;
	GError *error = NULL;
	uint32_t notification_id = 0;

	actions = variant_new_strv_fn(empty_actions, 0);
	hints = make_dbus_notification_hints(context);
	if (actions == NULL || hints == NULL) {
		if (actions != NULL)
			variant_unref_fn(actions);
		if (hints != NULL)
			variant_unref_fn(hints);
		return FB_SERVICE_FAILED;
	}
	parameters = variant_new_fn("(susss@as@a{sv}i)", context->app_name,
		context->notification_id, "", title, body, actions, hints,
		FB_NOTIFY_DBUS_DEFAULT_EXPIRE_MSEC);
	if (parameters == NULL)
		return FB_SERVICE_FAILED;
	reply = call_notification_server(context, "Notify", parameters, &error);
	if (error != NULL)
		error_free_fn(error);
	if (reply == NULL)
		return FB_SERVICE_UNAVAILABLE;
	variant_get_fn(reply, "(u)", &notification_id);
	variant_unref_fn(reply);
	if (notification_id == 0)
		return FB_SERVICE_FAILED;
	context->notification_id = notification_id;
	return FB_SERVICE_OK;
}

static int close_dbus_notification(FB_NOTIFY_UNIX *context)
{
	GVariant *parameters, *reply;
	GError *error = NULL;
	int result = FB_SERVICE_OK;
	if (context->notification_id == 0)
		return FB_SERVICE_OK;
	parameters = variant_new_fn("(u)", context->notification_id);
	if (parameters == NULL)
		return FB_SERVICE_FAILED;
	reply = call_notification_server(context, "CloseNotification",
		parameters, &error);
	if (error != NULL)
		error_free_fn(error);
	if (reply != NULL)
		variant_unref_fn(reply);
	else
		result = FB_SERVICE_UNAVAILABLE;
	context->notification_id = 0;
	return result;
}

static int close_luna_notification(FB_NOTIFY_UNIX *context)
{
	char response[FB_NOTIFY_LUNA_RESPONSE_MAX];
	char *payload;
	int result;
	if (context->toast_id[0] == '\0')
		return FB_SERVICE_OK;
	payload = make_luna_close_payload(context->toast_id);
	if (payload == NULL)
		return FB_SERVICE_FAILED;
	result = run_luna_send(context, FB_NOTIFY_LUNA_CLOSE_URI, payload,
		response, sizeof(response));
	free(payload);
	if (result != FB_SERVICE_OK)
		return result;
	if (!json_field_is_true(response, "returnValue"))
		return FB_SERVICE_UNAVAILABLE;
	context->toast_id[0] = '\0';
	return FB_SERVICE_OK;
}

static int send_luna_notification(FB_NOTIFY_UNIX *context,
	const char *title, const char *body)
{
	char response[FB_NOTIFY_LUNA_RESPONSE_MAX];
	char *payload;
	int result;
	if (context->toast_id[0] != '\0') {
		result = close_luna_notification(context);
		if (result != FB_SERVICE_OK)
			return result;
	}
	payload = make_luna_toast_payload(context, title, body);
	if (payload == NULL)
		return FB_SERVICE_FAILED;
	result = run_luna_send(context, FB_NOTIFY_LUNA_CREATE_URI, payload,
		response, sizeof(response));
	free(payload);
	if (result != FB_SERVICE_OK)
		return result;
	if (!json_field_is_true(response, "returnValue"))
		return FB_SERVICE_UNAVAILABLE;
	if (!json_copy_string_field(response, "toastId", context->toast_id,
		sizeof(context->toast_id)))
		context->toast_id[0] = '\0';
	return FB_SERVICE_OK;
}

static int send_notification(FB_NOTIFY_UNIX *context, const char *title, const char *body)
{
	GList *capabilities, *item;
	char *escaped = NULL;
	const char *formatted = body;
	int markup = FALSE;
	int result;
	if (context->luna_backend)
		return send_luna_notification(context, title, body);
	if (context->dbus_backend)
		return send_dbus_notification(context, title, body);
	capabilities = server_capabilities();
	for (item = capabilities; item != NULL; item = item->next) {
		if (item->data != NULL && strcmp(item->data, "body-markup") == 0)
			markup = TRUE;
		memory_free(item->data);
	}
	list_free(capabilities);
	/* A remote post is data. Escape for markup-capable servers; a plain-text
	 * server must receive the original bytes rather than visible XML entities.
	 */
	if (markup) {
		escaped = markup_escape(body, -1);
		if (escaped == NULL)
			return FB_SERVICE_FAILED;
		formatted = escaped;
	}
	if (context->notification == NULL)
		context->notification = notification_new(title, formatted, NULL);
	else if (!notification_update(context->notification, title, formatted, NULL)) {
		memory_free(escaped);
		return FB_SERVICE_FAILED;
	}
	memory_free(escaped);
	if (context->notification == NULL)
		return FB_SERVICE_FAILED;
	notification_app_name(context->notification, context->app_name);
	notification_hint_string(context->notification, "desktop-entry", context->app_id);
	result = notification_show(context->notification, NULL);
	return result ? FB_SERVICE_OK : FB_SERVICE_UNAVAILABLE;
}

int fb_hNotifySend(void *opaque, const char *title, const char *body)
{
	FB_NOTIFY_UNIX *context = opaque;
	int result;
	if (!pthread_equal(context->owner, pthread_self()))
		return FB_SERVICE_WRONG_THREAD;
	/* Separate owner threads may use separate contexts. libnotify's cached
	 * session proxy and initialization are shared, so serialize native calls
	 * as well as module references. Never hold a FreeBASIC lock here.
	 */
	if (!context->luna_backend)
		pthread_mutex_lock(&library_mutex);
	result = send_notification(context, title, body);
	if (!context->luna_backend)
		pthread_mutex_unlock(&library_mutex);
	return result;
}

int fb_hNotifyPoll(void *opaque)
{
	FB_NOTIFY_UNIX *context = opaque;
	return pthread_equal(context->owner, pthread_self()) ?
		FB_SERVICE_OK : FB_SERVICE_WRONG_THREAD;
}

int fb_hNotifyClose(void *opaque)
{
	FB_NOTIFY_UNIX *context = opaque;
	int result = FB_SERVICE_OK;
	if (!pthread_equal(context->owner, pthread_self()))
		return FB_SERVICE_WRONG_THREAD;
	if (context->luna_backend) {
		result = close_luna_notification(context);
	} else {
		pthread_mutex_lock(&library_mutex);
		if (context->dbus_backend) {
			result = close_dbus_notification(context);
		} else if (context->notification != NULL) {
			if (!notification_close(context->notification, NULL))
				result = FB_SERVICE_UNAVAILABLE;
			object_unref(context->notification);
			context->notification = NULL;
		}
		pthread_mutex_unlock(&library_mutex);
	}
	return result;
}

int fb_hNotifyDestroy(void *opaque)
{
	FB_NOTIFY_UNIX *context = opaque;
	if (!pthread_equal(context->owner, pthread_self()))
		return FB_SERVICE_WRONG_THREAD;
	/* Losing the session bus must not prevent local cleanup. */
	(void)fb_hNotifyClose(context);
	pthread_mutex_lock(&library_mutex);
	if (context->connection != NULL) {
		gobject_unref_fn(context->connection);
		context->connection = NULL;
	}
	if (context->library_reference) {
		release_notification_reference_locked();
		context->library_reference = FALSE;
	}
	pthread_mutex_unlock(&library_mutex);
	free(context);
	return FB_SERVICE_OK;
}

int fb_hNotifyPermission(void *opaque, int request)
{
	(void)request;
	return fb_hNotifyPoll(opaque);
}
#else
/* Darwin, Haiku and legacy Unix ports retain an honest unsupported result. */
#include "../../services/sys_hnotify.c"
#endif

/* end of drivers/unix/sys_hnotify.c */
