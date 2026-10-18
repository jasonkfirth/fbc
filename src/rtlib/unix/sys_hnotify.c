/*
 * FreeBASIC Unix runtime: sys_hnotify.c
 * Purpose: Present Linux and BSD desktop notifications using optional libnotify.
 * Responsibilities: Dynamic dependency, owner-thread contexts and plain text.
 * This file does not start a daemon, execute a shell or depend on X11/Wayland.
 */
#include "../fb_services.h"

#if defined(HOST_LINUX) || defined(HOST_FREEBSD) || defined(HOST_NETBSD) || \
    defined(HOST_OPENBSD) || defined(HOST_DRAGONFLY)
#include <dlfcn.h>
#include <pthread.h>

/* Use libnotify/GLib's actual opaque tags in dynamic function signatures.
 * Their object layouts stay private; calling through a differently typed
 * function pointer would not be a valid C interface even on a matching ABI.
 */
typedef struct _NotifyNotification NotifyNotification;
typedef struct _GError GError;

/* Stable GLib GList layout returned by notify_get_server_caps. Each data
 * string and the list itself are owned by the caller of that API.
 */
typedef struct _GList {
	void *data;
	struct _GList *next;
	struct _GList *prev;
} GList;

typedef struct FB_NOTIFY_UNIX {
	pthread_t owner;
	NotifyNotification *notification;
	char app_name[FB_NOTIFY_APP_MAX + 1u];
} FB_NOTIFY_UNIX;

/* libnotify has process-global initialization. Serialize our references and
 * keep its module loaded while any context exists. An application mixing
 * direct libnotify use must initialize it first and outlive these contexts.
 */
static pthread_mutex_t library_mutex = PTHREAD_MUTEX_INITIALIZER;
static void *library;
static unsigned int references;
static int own_init;
static int (*notify_init_fn)(const char *);
static int (*notify_is_initted_fn)(void);
static void (*notify_uninit_fn)(void);
static NotifyNotification *(*notification_new)(const char *, const char *, const char *);
static int (*notification_update)(NotifyNotification *, const char *, const char *, const char *);
static void (*notification_app_name)(NotifyNotification *, const char *);
static int (*notification_show)(NotifyNotification *, GError **);
static int (*notification_close)(NotifyNotification *, GError **);
static char *(*markup_escape)(const char *, ssize_t);
static void (*object_unref)(void *);
static void (*memory_free)(void *);
static GList *(*server_capabilities)(void);
static void (*list_free)(GList *);

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

int fb_hNotifyCreate(const char *app_name, void **output)
{
	FB_NOTIFY_UNIX *context = calloc(1, sizeof(*context));
	int ready = TRUE;
	if (context == NULL)
		return FB_SERVICE_FAILED;
	pthread_mutex_lock(&library_mutex);
	if (references == 0) {
		ready = load_library();
		if (ready) {
			own_init = !notify_is_initted_fn();
			if (own_init)
				ready = notify_init_fn(app_name);
		}
		if (!ready && library != NULL) {
			dlclose(library);
			library = NULL;
		}
	}
	if (ready && references != UINT_MAX)
		++references;
	else
		ready = FALSE;
	pthread_mutex_unlock(&library_mutex);
	if (!ready) {
		free(context);
		return FB_SERVICE_UNAVAILABLE;
	}
	context->owner = pthread_self();
	memcpy(context->app_name, app_name, strlen(app_name) + 1u);
	*output = context;
	return FB_SERVICE_OK;
}

static int send_notification(FB_NOTIFY_UNIX *context, const char *title, const char *body)
{
	GList *capabilities, *item;
	char *escaped = NULL;
	const char *formatted = body;
	int markup = FALSE;
	int result;
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
	pthread_mutex_lock(&library_mutex);
	result = send_notification(context, title, body);
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
	int result = TRUE;
	if (!pthread_equal(context->owner, pthread_self()))
		return FB_SERVICE_WRONG_THREAD;
	pthread_mutex_lock(&library_mutex);
	if (context->notification != NULL) {
		result = notification_close(context->notification, NULL);
		object_unref(context->notification);
		context->notification = NULL;
	}
	pthread_mutex_unlock(&library_mutex);
	return result ? FB_SERVICE_OK : FB_SERVICE_UNAVAILABLE;
}

int fb_hNotifyDestroy(void *opaque)
{
	FB_NOTIFY_UNIX *context = opaque;
	if (!pthread_equal(context->owner, pthread_self()))
		return FB_SERVICE_WRONG_THREAD;
	/* Losing the session bus must not prevent local cleanup. */
	(void)fb_hNotifyClose(context);
	free(context);
	pthread_mutex_lock(&library_mutex);
	if (--references == 0) {
		if (own_init)
			notify_uninit_fn();
		dlclose(library);
		library = NULL;
		own_init = FALSE;
	}
	pthread_mutex_unlock(&library_mutex);
	return FB_SERVICE_OK;
}

int fb_hNotifyPermission(void *opaque, int request)
{
	(void)request;
	return fb_hNotifyPoll(opaque);
}
#else
/* Darwin, Haiku and legacy Unix ports retain an honest unsupported result. */
#include "../sys_hnotify.c"
#endif
/* end of sys_hnotify.c */
