/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_debug.c

    Purpose:

        Implement the centralized bounded logging path for gfxlib3.

    Responsibilities:

        - synchronize callback and severity configuration
        - format messages into a fixed-size local buffer
        - provide conservative default error and warning output

    This file intentionally does NOT contain:

        - graphics API error conversion
        - logging from inner pixel or shader loops
        - persistent log-file management
*/

#include "gfx3_debug.h"
#include "gfx3_debug_platform.h"

static void fb_gfx3_default_log_callback(int level, const char *message,
	void *user_data)
{
	const char *label;

	(void)user_data;
	switch (level) {
	case FB_GFX3_LOG_ERROR:
		label = "error";
		break;
	case FB_GFX3_LOG_WARNING:
		label = "warning";
		break;
	case FB_GFX3_LOG_INFO:
		label = "info";
		break;
	default:
		label = "trace";
		break;
	}
	fb_gfx3_debug_platform_write(level, label, message);
}

int fb_gfx3_log_init(FB_GFX3_LOGGER *logger)
{
	if (logger == NULL)
		return FB_GFX3_INVALID;

	memset(logger, 0, sizeof(*logger));
	logger->mutex = fb_MutexCreate();
	if (logger->mutex == NULL)
		return FB_GFX3_OUT_OF_MEMORY;

	logger->callback = fb_gfx3_default_log_callback;
	logger->maximum_level = FB_GFX3_LOG_WARNING;
	return FB_GFX3_OK;
}

void fb_gfx3_log_destroy(FB_GFX3_LOGGER *logger)
{
	if (logger == NULL)
		return;
	if (logger->mutex != NULL)
		fb_MutexDestroy(logger->mutex);
	memset(logger, 0, sizeof(*logger));
}

int fb_gfx3_log_set(FB_GFX3_LOGGER *logger, int maximum_level,
	FB_GFX3_LOG_CALLBACK callback, void *user_data)
{
	if ((logger == NULL) || (logger->mutex == NULL) ||
	    (maximum_level < FB_GFX3_LOG_ERROR) ||
	    (maximum_level > FB_GFX3_LOG_TRACE))
		return FB_GFX3_INVALID;

	if (callback == NULL)
		callback = fb_gfx3_default_log_callback;

	fb_MutexLock(logger->mutex);
	logger->callback = callback;
	logger->user_data = user_data;
	logger->maximum_level = maximum_level;
	fb_MutexUnlock(logger->mutex);
	return FB_GFX3_OK;
}

void fb_gfx3_log_write(FB_GFX3_LOGGER *logger, int level,
	const char *format, ...)
{
	FB_GFX3_LOG_CALLBACK callback;
	void *user_data;
	char message[FB_GFX3_LOG_MESSAGE_SIZE];
	va_list arguments;
	int maximum_level;
	int length;

	if ((logger == NULL) || (logger->mutex == NULL) || (format == NULL) ||
	    (level < FB_GFX3_LOG_ERROR) || (level > FB_GFX3_LOG_TRACE))
		return;

	fb_MutexLock(logger->mutex);
	callback = logger->callback;
	user_data = logger->user_data;
	maximum_level = logger->maximum_level;
	fb_MutexUnlock(logger->mutex);

	if ((callback == NULL) || (level > maximum_level))
		return;

	va_start(arguments, format);
	length = vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	if (length < 0)
		return;
	message[sizeof(message) - 1] = '\0';
	callback(level, message, user_data);
}

/* end of gfx3_debug.c */
