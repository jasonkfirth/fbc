/*
 * Project: FreeBASIC Linux graphics support
 * File: fb_linux_device.c
 * Purpose: Classify known Linux mobile sessions and handheld hardware.
 * Responsibilities: Read stable session labels and standard chassis metadata.
 * This file intentionally does NOT contain window sizing or keyboard policy.
 */
#define _POSIX_C_SOURCE 200809L

#include "fb_linux_device.h"
#include "../mobile_control.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char linux_device_ascii_to_lower(unsigned char value)
{
	if ((value >= 'A') && (value <= 'Z'))
		return (unsigned char)(value + ('a' - 'A'));
	return value;
}

static int linux_device_ascii_is_space(unsigned char value)
{
	return (value == ' ') || (value == '\t') || (value == '\n') ||
		(value == '\r') || (value == '\f') || (value == '\v');
}

static int linux_device_text_equals_ignore_case(const char *left,
	const char *right)
{
	const unsigned char *left_byte = (const unsigned char *)left;
	const unsigned char *right_byte = (const unsigned char *)right;

	if (!left_byte || !right_byte)
		return 0;
	while (*left_byte && *right_byte) {
		if (linux_device_ascii_to_lower(*left_byte) !=
		    linux_device_ascii_to_lower(*right_byte))
			return 0;
		left_byte++;
		right_byte++;
	}
	return (*left_byte == '\0') && (*right_byte == '\0');
}

static int linux_device_value_is_true(const char *value)
{
	if (!value)
		return 0;
	return linux_device_text_equals_ignore_case(value, "1") ||
		linux_device_text_equals_ignore_case(value, "true") ||
		linux_device_text_equals_ignore_case(value, "yes") ||
		linux_device_text_equals_ignore_case(value, "on");
}

static int linux_device_token_equals(const char *start, size_t length,
	const char *expected)
{
	const unsigned char *left = (const unsigned char *)start;
	const unsigned char *right = (const unsigned char *)expected;
	size_t index = 0;

	while ((index < length) && right[index]) {
		if (linux_device_ascii_to_lower(left[index]) !=
		    linux_device_ascii_to_lower(right[index]))
			return 0;
		index++;
	}
	return (index == length) && (right[index] == '\0');
}

static int linux_device_has_mobile_session(const char *value)
{
	static const char *const labels[] = {
		"phosh", "lomiri", "ubuntu-touch", "unity8", "plasma-mobile",
		"gnome-mobile", "sxmo", "sailfish", "mobile", "handheld",
		"phone", "handset", "tablet", "hildon", "maemo", "luneos",
		"webos", "webos-ports", "mobian", "postmarketos"
	};
	const char *cursor = value;

	if (!cursor)
		return 0;
	while (*cursor) {
		const char *start;
		size_t length;
		size_t index;

		while (*cursor && ((*cursor == ':') || (*cursor == ';') ||
		       (*cursor == ',') ||
		       linux_device_ascii_is_space((unsigned char)*cursor)))
			cursor++;
		start = cursor;
		while (*cursor && (*cursor != ':') && (*cursor != ';') &&
		       (*cursor != ',') &&
		       !linux_device_ascii_is_space((unsigned char)*cursor))
			cursor++;
		length = (size_t)(cursor - start);
		for (index = 0; index < sizeof(labels) / sizeof(labels[0]); index++) {
			if (linux_device_token_equals(start, length, labels[index]))
				return 1;
		}
	}
	return 0;
}

static int linux_device_has_desktop_session(const char *value)
{
	static const char *const labels[] = {
		"gnome", "kde", "plasma", "ubuntu", "xfce", "xfce4", "mate",
		"cinnamon", "lxde", "lxqt", "budgie", "pantheon", "deepin", "cosmic"
	};
	const char *cursor = value;

	if (!cursor)
		return 0;
	while (*cursor) {
		const char *start;
		size_t length;
		size_t index;

		while (*cursor && ((*cursor == ':') || (*cursor == ';') ||
		       (*cursor == ',') ||
		       linux_device_ascii_is_space((unsigned char)*cursor)))
			cursor++;
		start = cursor;
		while (*cursor && (*cursor != ':') && (*cursor != ';') &&
		       (*cursor != ',') &&
		       !linux_device_ascii_is_space((unsigned char)*cursor))
			cursor++;
		length = (size_t)(cursor - start);
		for (index = 0; index < sizeof(labels) / sizeof(labels[0]); index++) {
			if (linux_device_token_equals(start, length, labels[index]))
				return 1;
		}
	}
	return 0;
}

/*
 * The device-tree specification and systemd's machine-info use the same
 * chassis names. Convertibles follow the existing SMBIOS tablet policy; an
 * application or user can still choose desktop behavior with the override.
 */
static int linux_device_chassis_type(const char *value, size_t length)
{
	static const char *const handheld[] = {
		"handset", "tablet", "watch", "convertible"
	};
	static const char *const desktop[] = {
		"desktop", "laptop", "server"
	};
	size_t index;

	for (index = 0; index < sizeof(handheld) / sizeof(handheld[0]); index++) {
		if (linux_device_token_equals(value, length, handheld[index]))
			return FB_GFX_DEVICE_CLASS_HANDHELD;
	}
	for (index = 0; index < sizeof(desktop) / sizeof(desktop[0]); index++) {
		if (linux_device_token_equals(value, length, desktop[index]))
			return FB_GFX_DEVICE_CLASS_DESKTOP;
	}
	return FB_GFX_DEVICE_CLASS_UNKNOWN;
}

/*
 * machine-info is an administrator-provided environment-style file. Only the
 * CHASSIS value is needed here; it is a fixed word, not a shell expression.
 * Reject incomplete lines and trailing text rather than evaluating the file.
 */
static int linux_device_machine_info_class(void)
{
	FILE *file = fopen("/etc/machine-info", "r");
	char line[256];
	int result = FB_GFX_DEVICE_CLASS_UNKNOWN;

	if (!file)
		return result;
	while (fgets(line, sizeof(line), file)) {
		char *cursor = line;
		char *start, *end;
		int quote = 0;

		if (!strchr(line, '\n') && !feof(file)) {
			int byte;
			while ((byte = fgetc(file)) != '\n' && byte != EOF)
				;
			continue;
		}
		while (linux_device_ascii_is_space((unsigned char)*cursor))
			cursor++;
		if (strncmp(cursor, "CHASSIS", 7) != 0)
			continue;
		cursor += 7;
		while (linux_device_ascii_is_space((unsigned char)*cursor))
			cursor++;
		if (*cursor != '=')
			continue;
		cursor++;
		while (linux_device_ascii_is_space((unsigned char)*cursor))
			cursor++;
		if (*cursor == '\'' || *cursor == '"')
			quote = (unsigned char)*cursor++;
		start = cursor;
		if (quote) {
			while (*cursor && *cursor != quote)
				cursor++;
			if (*cursor != quote)
				continue;
			end = cursor++;
		} else {
			while (*cursor &&
			       !linux_device_ascii_is_space((unsigned char)*cursor))
				cursor++;
			end = cursor;
		}
		while (linux_device_ascii_is_space((unsigned char)*cursor))
			cursor++;
		if (*cursor && *cursor != '#')
			continue;
		result = linux_device_chassis_type(start, (size_t)(end - start));
	}
	fclose(file);
	return result;
}

/* Device-tree strings include a terminating NUL; do not accept trailing data. */
static int linux_device_tree_chassis_class(void)
{
	static const char *const paths[] = {
		"/sys/firmware/devicetree/base/chassis-type",
		"/proc/device-tree/chassis-type"
	};
	size_t index;

	for (index = 0; index < sizeof(paths) / sizeof(paths[0]); index++) {
		FILE *file = fopen(paths[index], "rb");
		char value[32];
		size_t count, length;
		int failed;

		if (!file)
			continue;
		count = fread(value, 1, sizeof(value), file);
		failed = ferror(file) || count == 0 || count == sizeof(value);
		fclose(file);
		if (failed)
			continue;
		for (length = 0; length < count && value[length] != '\0'; length++)
			;
		if (length < count && length + 1 != count)
			continue;
		return linux_device_chassis_type(value, length);
	}
	return FB_GFX_DEVICE_CLASS_UNKNOWN;
}

static int linux_device_chassis_is_handheld(void)
{
	static const char *const paths[] = {
		"/sys/class/dmi/id/chassis_type",
		"/sys/devices/virtual/dmi/id/chassis_type"
	};
	size_t index;

	for (index = 0; index < sizeof(paths) / sizeof(paths[0]); index++) {
		FILE *file = fopen(paths[index], "r");
		char value[32];
		char *end;
		long chassis_type;

		if (!file)
			continue;
		if (!fgets(value, sizeof(value), file)) {
			fclose(file);
			continue;
		}
		fclose(file);
		chassis_type = strtol(value, &end, 10);
		while (*end && linux_device_ascii_is_space((unsigned char)*end))
			end++;
		if (*end != '\0')
			continue;
		/* SMBIOS 11 is handheld; 30-32 cover tablet/convertible forms. */
		if ((chassis_type == 11) || (chassis_type == 30) ||
		    (chassis_type == 31) || (chassis_type == 32))
			return 1;
	}
	return 0;
}

int fb_hLinuxDeviceClass(void)
{
	const char *override = getenv("FBC_DEVICE_CLASS");
	const char *session_names[] = {
		"XDG_CURRENT_DESKTOP", "XDG_SESSION_DESKTOP", "DESKTOP_SESSION",
		"GNOME_SHELL_SESSION_MODE", "GDMSESSION", "PLASMA_PLATFORM"
	};
	size_t index;
	int chassis;

	if (override) {
		if (linux_device_text_equals_ignore_case(override, "handheld") ||
		    linux_device_text_equals_ignore_case(override, "mobile") ||
		    linux_device_text_equals_ignore_case(override, "tablet"))
			return FB_GFX_DEVICE_CLASS_HANDHELD;
		if (linux_device_text_equals_ignore_case(override, "desktop"))
			return FB_GFX_DEVICE_CLASS_DESKTOP;
		if (linux_device_text_equals_ignore_case(override, "unknown"))
			return FB_GFX_DEVICE_CLASS_UNKNOWN;
	}

	chassis = linux_device_machine_info_class();
	if (chassis != FB_GFX_DEVICE_CLASS_UNKNOWN)
		return chassis;
	/* Sailfish and Maemo expose their mobile session through the Qt IM module. */
	if (linux_device_text_equals_ignore_case(getenv("QT_IM_MODULE"), "maliit") ||
	    linux_device_text_equals_ignore_case(getenv("QT_IM_MODULE"), "him"))
		return FB_GFX_DEVICE_CLASS_HANDHELD;
	if (linux_device_value_is_true(getenv("QT_QUICK_CONTROLS_MOBILE")) ||
	    linux_device_value_is_true(getenv("PLASMA_MOBILE")) ||
	    linux_device_value_is_true(getenv("FBC_MOBILE_MODE")))
		return FB_GFX_DEVICE_CLASS_HANDHELD;
	for (index = 0; index < sizeof(session_names) / sizeof(session_names[0]);
	     index++) {
		if (linux_device_has_mobile_session(getenv(session_names[index])))
			return FB_GFX_DEVICE_CLASS_HANDHELD;
	}
	chassis = linux_device_tree_chassis_class();
	if (chassis != FB_GFX_DEVICE_CLASS_UNKNOWN)
		return chassis;
	if (linux_device_chassis_is_handheld())
		return FB_GFX_DEVICE_CLASS_HANDHELD;
	for (index = 0; index < sizeof(session_names) / sizeof(session_names[0]);
	     index++) {
		if (linux_device_has_desktop_session(getenv(session_names[index])))
			return FB_GFX_DEVICE_CLASS_DESKTOP;
	}
	return FB_GFX_DEVICE_CLASS_UNKNOWN;
}

/* end of fb_linux_device.c */
