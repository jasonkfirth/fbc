/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_debug.h

    Purpose:

        Provide one bounded and configurable logging path for all gfxlib3
        modules and renderer backends.

    Responsibilities:

        - define stable log severity levels
        - protect logger configuration shared by multiple threads
        - keep debug output behind one callback

    This file intentionally does NOT contain:

        - per-pixel tracing
        - platform debugger integration
        - renderer error-state policy
*/

#ifndef __FB_GFX3_DEBUG_H__
#define __FB_GFX3_DEBUG_H__

#include "fb_gfx3.h"

#define FB_GFX3_LOG_MESSAGE_SIZE 1024u

enum FB_GFX3_LOG_LEVEL {
	FB_GFX3_LOG_ERROR = 0,
	FB_GFX3_LOG_WARNING,
	FB_GFX3_LOG_INFO,
	FB_GFX3_LOG_TRACE
};

typedef void (*FB_GFX3_LOG_CALLBACK)(int level, const char *message,
	void *user_data);

typedef struct FB_GFX3_LOGGER {
	FBMUTEX *mutex;
	FB_GFX3_LOG_CALLBACK callback;
	void *user_data;
	int maximum_level;
} FB_GFX3_LOGGER;

int fb_gfx3_log_init(FB_GFX3_LOGGER *logger);
void fb_gfx3_log_destroy(FB_GFX3_LOGGER *logger);
int fb_gfx3_log_set(FB_GFX3_LOGGER *logger, int maximum_level,
	FB_GFX3_LOG_CALLBACK callback, void *user_data);
#if defined(__GNUC__)
__attribute__((format(printf, 3, 4)))
#endif
void fb_gfx3_log_write(FB_GFX3_LOGGER *logger, int level,
	const char *format, ...);

#endif

/* end of gfx3_debug.h */
