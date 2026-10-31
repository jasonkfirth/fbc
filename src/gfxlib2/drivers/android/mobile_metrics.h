/*
 * FreeBASIC Android graphics: mobile_metrics.h
 * Purpose: Share Java-observed window and IME state between gfxlib2 and gfxlib3.
 * Responsibilities: Synchronize callback snapshots and dispatch mobile controls.
 * This file does not render, retain activities or attach native threads to ART.
 * Each graphics library includes this implementation in its Android driver;
 * an application links one graphics library, so the JNI entry point is unique.
 */
#ifndef FB_ANDROID_MOBILE_METRICS_H
#define FB_ANDROID_MOBILE_METRICS_H
#include "../mobile_control.h"

static pthread_mutex_t mobile_metrics_mutex = PTHREAD_MUTEX_INITIALIZER;
static int mobile_width, mobile_height, mobile_dpi, mobile_rotation;
static int mobile_keyboard_visible = -2;
static int mobile_available;
static int mobile_managed_input;

JNIEXPORT void JNICALL
Java_org_freebasic_android_FreeBasicNativeActivity_nativeInputViewReady(
	JNIEnv *environment, jclass class_object, jboolean ready)
{
	(void)environment;
	(void)class_object;
	pthread_mutex_lock(&mobile_metrics_mutex);
	mobile_managed_input = ready == JNI_TRUE;
	pthread_mutex_unlock(&mobile_metrics_mutex);
}

/* Implemented by the owning driver. This is an explicit show/hide request,
 * never a toggle: asynchronous layout callbacks may arrive between requests. */
static void android_request_soft_keyboard(int visible);

/* Fullscreen activities may keep a full-height surface behind the IME. An
 * application which selected the exact usable size needs a top-left viewport,
 * rather than scaling or centering that page behind the keyboard. Lock order
 * is driver state then metrics; the Java snapshot callback takes only metrics. */
static void android_mobile_viewport(int source_width, int source_height,
	int *width, int *height)
{
	pthread_mutex_lock(&mobile_metrics_mutex);
	if (mobile_available && source_width == mobile_width &&
	    source_height == mobile_height && mobile_width <= *width &&
	    mobile_height <= *height) {
		*width = mobile_width;
		*height = mobile_height;
	}
	pthread_mutex_unlock(&mobile_metrics_mutex);
}

JNIEXPORT void JNICALL
Java_org_freebasic_android_FreeBasicNativeActivity_nativeWindowMetrics(
	JNIEnv *environment, jclass class_object, jint width, jint height,
	jint dpi, jint rotation, jint keyboard_visible)
{
	(void)environment;
	(void)class_object;
	pthread_mutex_lock(&mobile_metrics_mutex);
	mobile_width = (width > 0 && width <= 32767) ? width : 0;
	mobile_height = (height > 0 && height <= 32767) ? height : 0;
	mobile_dpi = (dpi >= 72 && dpi <= 1920) ? dpi : 160;
	mobile_rotation = (rotation >= 0 && rotation < 360) ? rotation : 0;
	mobile_keyboard_visible = keyboard_visible;
	mobile_available = width > 0 && height > 0;
	pthread_mutex_unlock(&mobile_metrics_mutex);
}

void fb_hMobileControl(int what, ssize_t *p1, ssize_t *p2,
	ssize_t *p3, ssize_t *p4)
{
	if (what == SET_SOFT_KEYBOARD) {
		if (*p1 != (ssize_t)0x80000000)
			android_request_soft_keyboard(*p1 != 0);
		return;
	}
	if (what != GET_WINDOW_METRICS && what != GET_SOFT_KEYBOARD)
		return;
	pthread_mutex_lock(&mobile_metrics_mutex);
	if (what == GET_WINDOW_METRICS) {
		*p1 = mobile_width; *p2 = mobile_height;
		*p3 = mobile_dpi; *p4 = mobile_rotation;
	} else {
		*p1 = mobile_keyboard_visible;
		*p2 = mobile_available ? -1 : 0;
		*p3 = *p4 = 0;
	}
	pthread_mutex_unlock(&mobile_metrics_mutex);
}
#endif
/* end of mobile_metrics.h */
