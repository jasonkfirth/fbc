/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: android/gfx3_debug_platform.c

    Purpose:

        Route default gfxlib3 diagnostics through Android logcat.

    Responsibilities:

        - map centralized gfxlib3 severities to Android priorities
        - preserve the bounded logger label and message

    This file intentionally does NOT contain:

        - message formatting buffers
        - NativeActivity lifecycle handling
        - graphics API error conversion
*/

#include "../gfx3_debug_platform.h"

#include <android/log.h>

void fb_gfx3_debug_platform_write(int level, const char *label,
	const char *message)
{
	int priority;

	if ((label == NULL) || (message == NULL))
		return;
	if (level == FB_GFX3_LOG_ERROR)
		priority = ANDROID_LOG_ERROR;
	else if (level == FB_GFX3_LOG_WARNING)
		priority = ANDROID_LOG_WARN;
	else
		priority = ANDROID_LOG_INFO;
	__android_log_print(priority, "gfxlib3", "%s: %s", label, message);
}

/* end of android/gfx3_debug_platform.c */
