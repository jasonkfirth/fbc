/*
 * Project: FreeBASIC Unix notification tests
 * File: notify-desktop-entry.c
 * Purpose: Check desktop identity through both freedesktop backends.
 * Responsibilities: Exercise libnotify callbacks and real GVariant encoding.
 * This file does not connect to a desktop service or display notifications.
 */
#include "../../src/rtlib/drivers/unix/sys_hnotify.c"

static unsigned hints_set, shows, updates;

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "notify-desktop-entry: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

static NotifyNotification *create_notification(const char *title, const char *body, const char *icon)
{
	(void)title; (void)body; (void)icon;
	return (NotifyNotification *)(uintptr_t)1;
}

static int update_notification(NotifyNotification *notification, const char *title, const char *body, const char *icon)
{
	(void)notification; (void)title; (void)body; (void)icon;
	updates++;
	return 1;
}

static void set_app_name(NotifyNotification *notification, const char *app_name)
{
	(void)notification;
	require(strcmp(app_name, "Templari") == 0, "display name changed");
}

static void set_hint(NotifyNotification *notification, const char *key, const char *value)
{
	(void)notification;
	require(strcmp(key, "desktop-entry") == 0 && strcmp(value, "templari-native-qa") == 0,
		"libnotify identity missing");
	hints_set++;
}

static int show_notification(NotifyNotification *notification, GError **error)
{
	(void)notification; (void)error;
	require(hints_set == shows + 1, "show preceded identity");
	shows++;
	return 1;
}

static GList *no_capabilities(void) { return NULL; }
static void free_list(GList *list) { require(list == NULL, "unexpected capabilities"); }

int main(void)
{
	FB_NOTIFY_UNIX context;
	memset(&context, 0, sizeof(context));
	strcpy(context.app_name, "Templari");
	strcpy(context.app_id, "templari-native-qa");
	notification_new = create_notification;
	notification_update = update_notification;
	notification_app_name = set_app_name;
	notification_hint_string = set_hint;
	notification_show = show_notification;
	server_capabilities = no_capabilities;
	list_free = free_list;
	memory_free = free;
	require(send_notification(&context, "Title", "Body") == FB_SERVICE_OK, "creation failed");
	require(send_notification(&context, "Title", "Body") == FB_SERVICE_OK &&
		updates == 1 && hints_set == 2, "update lost identity");
	/* The installed GLib validates the dictionary shape and reference ownership. */
	require(load_gdbus_libraries(), "GIO could not be loaded");
	GVariant *(*lookup)(GVariant *, const char *, const GVariantType *);
	void *address = dlsym(glib_library, "g_variant_lookup_value");
	require(address != NULL && sizeof(lookup) == sizeof(address), "lookup unavailable");
	memcpy(&lookup, &address, sizeof(lookup));
	GVariant *hints = make_dbus_notification_hints(&context);
	require(hints != NULL, "GIO hints not encoded");
	GVariant *identity = lookup(hints, "desktop-entry", (const GVariantType *)"s");
	require(identity != NULL, "GIO identity missing or wrong type");
	const char *text = NULL;
	variant_get_fn(identity, "&s", &text);
	require(text != NULL && strcmp(text, context.app_id) == 0, "GIO identity changed");
	variant_unref_fn(identity);
	variant_unref_fn(hints);
	unload_gdbus_libraries();
	puts("notify-desktop-entry: PASS libnotify-create/update GIO-real-variant");
	return EXIT_SUCCESS;
}

/* end of notify-desktop-entry.c */
