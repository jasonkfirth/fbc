/*
 * FreeBASIC Android graphics tests
 * File: android-mobile-metrics.c
 * Purpose: Check the shared production window/IME snapshot contract.
 * Responsibilities: First-show reservation, overlap clipping, rotation,
 * floating/unknown visibility, bounds and explicit keyboard requests.
 * This file does not create a window, replace JNI or emulate an IME.
 * Compile with the Android NDK for API 24 and run on the selected device.
 */
#define HOST_ANDROID 1
#include <jni.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../src/gfxlib2/drivers/android/mobile_metrics.h"

static unsigned checks;
static int last_request = -1;

static void android_request_soft_keyboard(int visible)
{
	last_request = visible;
}

static void check(int value, const char *expression, int line)
{
	checks++;
	if (!value) {
		fprintf(stderr, "android-mobile-metrics:%d: %s\n", line, expression);
		exit(EXIT_FAILURE);
	}
}
#define CHECK(expression) check((expression), #expression, __LINE__)

static void observe(int width, int height, int dpi, int visible)
{
	Java_org_freebasic_android_FreeBasicNativeActivity_nativeWindowMetrics(
		NULL, NULL, width, height, dpi, 0, visible);
}

static void keyboard(ssize_t *visible, ssize_t *available,
	ssize_t *top, ssize_t *height)
{
	*visible = *available = *top = *height = 123;
	fb_hMobileControl(GET_SOFT_KEYBOARD, visible, available, top, height);
}

int main(void)
{
	ssize_t visible, available, top, height;
	int width, window_height;

	keyboard(&visible, &available, &top, &height);
	CHECK(visible == -2 && available == 0 && top == 0 && height == 0);

	/* The first snapshot already has an IME. No earlier full-height Java
	 * snapshot is needed when presentation measured the native buffer. */
	observe(720, 760, 320, -1);
	width = 720; window_height = 1280;
	android_mobile_viewport(720, 760, &width, &window_height);
	CHECK(width == 720 && window_height == 760);
	keyboard(&visible, &available, &top, &height);
	CHECK(visible == -1 && available == -1);
	CHECK(top == 760 && height == 0);

	/* An unresized BASIC page still overlaps the measured keyboard. */
	width = 720; window_height = 1280;
	android_mobile_viewport(720, 1280, &width, &window_height);
	keyboard(&visible, &available, &top, &height);
	CHECK(top == 760 && height == 520);
	/* Arbitrary fixed pages can be centered or scaled by presentation. */
	width = 720; window_height = 1280;
	android_mobile_viewport(720, 500, &width, &window_height);
	keyboard(&visible, &available, &top, &height);
	CHECK(top == 0 && height == 0);

	observe(720, 760, 320, 0);
	keyboard(&visible, &available, &top, &height);
	CHECK(visible == 0 && top == 0 && height == 0);
	observe(720, 760, 320, -2);
	keyboard(&visible, &available, &top, &height);
	CHECK(visible == -2 && top == 0 && height == 0);

	/* Visible floating keyboards and ordinary system chrome do not prove
	 * a docked keyboard rectangle. Retain the consumer's fallback. */
	observe(720, 1240, 320, -1);
	width = 720; window_height = 1280;
	android_mobile_viewport(720, 1240, &width, &window_height);
	keyboard(&visible, &available, &top, &height);
	CHECK(top == 0 && height == 0);

	/* Rotation must not combine a new width with the old native buffer. */
	observe(1280, 500, 320, -1);
	keyboard(&visible, &available, &top, &height);
	CHECK(top == 0 && height == 0);
	width = 1280; window_height = 720;
	android_mobile_viewport(1280, 500, &width, &window_height);
	keyboard(&visible, &available, &top, &height);
	CHECK(top == 500 && height == 0);

	/* A page with a different scale cannot use native-pixel bounds directly. */
	width = 1280; window_height = 720;
	android_mobile_viewport(640, 500, &width, &window_height);
	keyboard(&visible, &available, &top, &height);
	CHECK(top == 0 && height == 0);
	android_mobile_viewport(1280, 500, NULL, &window_height);
	android_mobile_viewport(1280, 500, &width, NULL);

	observe(40000, 500, 320, -1);
	keyboard(&visible, &available, &top, &height);
	CHECK(available == 0 && top == 0 && height == 0);
	observe(1280, -1, 320, -1);
	keyboard(&visible, &available, &top, &height);
	CHECK(available == 0 && top == 0 && height == 0);

	observe(1280, 500, 320, -1);
	width = 40000; window_height = 720;
	android_mobile_viewport(1280, 500, &width, &window_height);
	keyboard(&visible, &available, &top, &height);
	CHECK(top == 0 && height == 0);

	visible = 1;
	fb_hMobileControl(SET_SOFT_KEYBOARD, &visible, &available, &top, &height);
	CHECK(last_request == 1);
	visible = 0;
	fb_hMobileControl(SET_SOFT_KEYBOARD, &visible, &available, &top, &height);
	CHECK(last_request == 0);
	visible = (ssize_t)0x80000000;
	fb_hMobileControl(SET_SOFT_KEYBOARD, &visible, &available, &top, &height);
	CHECK(last_request == 0);

	printf("ANDROID_MOBILE_METRICS_PASS checks=%u\n", checks);
	return EXIT_SUCCESS;
}
/* end of android-mobile-metrics.c */
