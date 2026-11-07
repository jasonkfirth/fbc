/*
 * Project: FreeBASIC Linux graphics tests
 * File: linux-device-class.c
 * Purpose: Check Linux device classification without depending on host hardware.
 * Responsibilities: Exercise session labels and standard chassis metadata.
 * This file intentionally does NOT contain display or input integration tests.
 *
 * Build and run:
 *   cc -std=c11 -Wall -Wextra -Werror tests/gfx/linux-device-class.c -o /tmp/linux-device-class
 *   /tmp/linux-device-class
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *test_machine_info;
static const unsigned char *test_device_tree;
static size_t test_device_tree_size;
static const char *test_dmi_chassis;

/* Supply only the selected fixture; never read the test host's own chassis. */
static FILE *test_chassis_open(const char *path, const char *mode)
{
	const void *contents = NULL;
	size_t length = 0;
	FILE *file;
	(void)mode;
	if (strcmp(path, "/etc/machine-info") == 0 && test_machine_info) {
		contents = test_machine_info;
		length = strlen(test_machine_info);
	} else if ((strcmp(path, "/sys/firmware/devicetree/base/chassis-type") == 0 ||
	            strcmp(path, "/proc/device-tree/chassis-type") == 0) &&
	           test_device_tree) {
		contents = test_device_tree;
		length = test_device_tree_size;
	} else if ((strcmp(path, "/sys/class/dmi/id/chassis_type") == 0 ||
	            strcmp(path, "/sys/devices/virtual/dmi/id/chassis_type") == 0) &&
	           test_dmi_chassis) {
		contents = test_dmi_chassis;
		length = strlen(test_dmi_chassis);
	}
	if (!contents)
		return NULL;
	file = tmpfile();
	if (!file || fwrite(contents, 1, length, file) != length) {
		perror("chassis fixture");
		exit(EXIT_FAILURE);
	}
	rewind(file);
	return file;
}

#define fopen test_chassis_open
#include "../../src/gfxlib2/drivers/linux/fb_linux_device.c"
#undef fopen

static void clear_device_environment(void)
{
	static const char *const names[] = {
		"FBC_DEVICE_CLASS", "MIR_SOCKET", "QT_QUICK_CONTROLS_MOBILE",
		"PLASMA_MOBILE", "FBC_MOBILE_MODE", "PLASMA_PLATFORM",
		"XDG_CURRENT_DESKTOP", "XDG_SESSION_DESKTOP", "DESKTOP_SESSION",
		"GNOME_SHELL_SESSION_MODE", "GDMSESSION", "QT_IM_MODULE"
	};
	size_t index;

	for (index = 0; index < sizeof(names) / sizeof(names[0]); index++) {
		if (unsetenv(names[index]) != 0) {
			perror("unsetenv");
			exit(EXIT_FAILURE);
		}
	}
	test_machine_info = NULL;
	test_device_tree = NULL;
	test_device_tree_size = 0;
	test_dmi_chassis = NULL;
}

static void set_test_environment(const char *name, const char *value)
{
	if (setenv(name, value, 1) != 0) {
		perror("setenv");
		exit(EXIT_FAILURE);
	}
}

static void expect_device_class(const char *name, int expected)
{
	int actual = fb_hLinuxDeviceClass();

	if (actual != expected) {
		fprintf(stderr, "%s: expected %d, got %d\n", name, expected, actual);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	static const unsigned char handset[] = "handset";
	static const unsigned char tablet[] = "tablet";
	static const unsigned char invalid_tree[] = "handset\0extra";
	static const unsigned char long_tree[] =
		"handset-identifier-that-is-far-too-long";
	char long_line[320];

	clear_device_environment();
	set_test_environment("XDG_CURRENT_DESKTOP", "KDE");
	set_test_environment("PLASMA_PLATFORM", "phone:handset");
	expect_device_class("Plasma phone", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("XDG_CURRENT_DESKTOP", "KDE");
	set_test_environment("PLASMA_PLATFORM", "handset");
	expect_device_class("Plasma handset", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("XDG_CURRENT_DESKTOP", "KDE");
	set_test_environment("PLASMA_PLATFORM", "bigphone:handsetxyz");
	expect_device_class("unrelated Plasma platform", FB_GFX_DEVICE_CLASS_DESKTOP);

	clear_device_environment();
	set_test_environment("XDG_CURRENT_DESKTOP", "KDE");
	set_test_environment("PLASMA_PLATFORM", "desktop");
	set_test_environment("MIR_SOCKET", "/run/user/1000/mir_socket");
	expect_device_class("Mir desktop", FB_GFX_DEVICE_CLASS_DESKTOP);

	clear_device_environment();
	set_test_environment("MIR_SOCKET", "/run/user/1000/mir_socket");
	expect_device_class("Mir without a session marker", FB_GFX_DEVICE_CLASS_UNKNOWN);

	clear_device_environment();
	set_test_environment("DESKTOP_SESSION", "ubuntu-touch");
	set_test_environment("MIR_SOCKET", "/run/user/32011/mir_socket");
	expect_device_class("Ubuntu Touch", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("XDG_CURRENT_DESKTOP", "GNOME");
	set_test_environment("GNOME_SHELL_SESSION_MODE", "gnome-mobile");
	expect_device_class("GNOME Mobile", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("QT_IM_MODULE", "him");
	expect_device_class("Maemo Hildon input method",
		FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("XDG_CURRENT_DESKTOP", "Hildon");
	expect_device_class("Hildon desktop", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("XDG_CURRENT_DESKTOP", "LuneOS");
	expect_device_class("LuneOS desktop", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("XDG_CURRENT_DESKTOP", "webOS");
	expect_device_class("webOS desktop", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("DESKTOP_SESSION", "mobian");
	expect_device_class("Mobian desktop", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("DESKTOP_SESSION", "postmarketos");
	expect_device_class("postmarketOS desktop", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	set_test_environment("FBC_DEVICE_CLASS", "desktop");
	set_test_environment("PLASMA_PLATFORM", "phone:handset");
	expect_device_class("explicit desktop override", FB_GFX_DEVICE_CLASS_DESKTOP);

	clear_device_environment();
	test_machine_info = "PRETTY_HOSTNAME=Phone\nCHASSIS=\"handset\"\n";
	set_test_environment("XDG_CURRENT_DESKTOP", "GNOME");
	expect_device_class("machine-info handset", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	test_machine_info = " CHASSIS = 'tablet' # configured form factor\n";
	expect_device_class("machine-info tablet", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	test_machine_info = "CHASSIS=desktop\n";
	test_device_tree = handset;
	test_device_tree_size = sizeof(handset);
	set_test_environment("XDG_CURRENT_DESKTOP", "Phosh");
	expect_device_class("configured desktop chassis", FB_GFX_DEVICE_CLASS_DESKTOP);
	set_test_environment("FBC_DEVICE_CLASS", "handheld");
	expect_device_class("override configured chassis", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	test_device_tree = handset;
	test_device_tree_size = sizeof(handset);
	set_test_environment("XDG_CURRENT_DESKTOP", "GNOME");
	expect_device_class("device-tree handset", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	test_machine_info = "CHASSIS=embedded\n";
	test_device_tree = tablet;
	test_device_tree_size = sizeof(tablet);
	expect_device_class("device-tree tablet", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	test_machine_info = "CHASSIS=\"handset\" extra\n";
	set_test_environment("XDG_CURRENT_DESKTOP", "GNOME");
	expect_device_class("invalid machine-info suffix", FB_GFX_DEVICE_CLASS_DESKTOP);
	test_machine_info = "CHASSIS=handset#extra\n";
	expect_device_class("invalid unquoted chassis", FB_GFX_DEVICE_CLASS_DESKTOP);

	clear_device_environment();
	memset(long_line, ' ', sizeof(long_line));
	memcpy(long_line, "CHASSIS=handset", 15);
	long_line[sizeof(long_line) - 2] = '\n';
	long_line[sizeof(long_line) - 1] = '\0';
	test_machine_info = long_line;
	set_test_environment("XDG_CURRENT_DESKTOP", "GNOME");
	expect_device_class("overlong machine-info line", FB_GFX_DEVICE_CLASS_DESKTOP);

	clear_device_environment();
	test_device_tree = invalid_tree;
	test_device_tree_size = sizeof(invalid_tree);
	set_test_environment("XDG_CURRENT_DESKTOP", "GNOME");
	expect_device_class("invalid device-tree suffix", FB_GFX_DEVICE_CLASS_DESKTOP);
	test_device_tree = long_tree;
	test_device_tree_size = sizeof(long_tree);
	expect_device_class("overlong device-tree value", FB_GFX_DEVICE_CLASS_DESKTOP);

	clear_device_environment();
	test_dmi_chassis = "11\n";
	set_test_environment("XDG_CURRENT_DESKTOP", "GNOME");
	expect_device_class("SMBIOS handheld", FB_GFX_DEVICE_CLASS_HANDHELD);

	clear_device_environment();
	puts("LINUX_DEVICE_CLASS_PASS");
	return EXIT_SUCCESS;
}

/* end of linux-device-class.c */
