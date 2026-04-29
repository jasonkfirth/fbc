#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_android.h"

#include <aaudio/AAudio.h>
#include <android/api-level.h>
#include <dlfcn.h>
#include <stdint.h>
#include <string.h>

typedef struct FB_SFX_AAUDIO_API
{
	void *lib;
	aaudio_result_t (*createStreamBuilder)(AAudioStreamBuilder **builder);
	void (*deleteStreamBuilder)(AAudioStreamBuilder *builder);
	void (*setDirection)(AAudioStreamBuilder *builder, aaudio_direction_t direction);
	void (*setSampleRate)(AAudioStreamBuilder *builder, int32_t sample_rate);
	void (*setChannelCount)(AAudioStreamBuilder *builder, int32_t channel_count);
	void (*setFormat)(AAudioStreamBuilder *builder, aaudio_format_t format);
	void (*setPerformanceMode)(AAudioStreamBuilder *builder, aaudio_performance_mode_t mode);
	void (*setSharingMode)(AAudioStreamBuilder *builder, aaudio_sharing_mode_t mode);
	aaudio_result_t (*openStream)(AAudioStreamBuilder *builder, AAudioStream **stream);
	aaudio_result_t (*requestStart)(AAudioStream *stream);
	aaudio_result_t (*requestStop)(AAudioStream *stream);
	aaudio_result_t (*close)(AAudioStream *stream);
	aaudio_result_t (*write)(AAudioStream *stream, const void *buffer, int32_t frames, int64_t timeout_nanoseconds);
} FB_SFX_AAUDIO_API;

static FB_SFX_AAUDIO_API api;
static AAudioStream *stream = NULL;
static int initialized = 0;

static void *load_symbol(const char *name)
{
	return api.lib ? dlsym(api.lib, name) : NULL;
}

static int load_api(void)
{
	if (api.lib)
		return 0;

	if (android_get_device_api_level() < 26)
		return -1;

	api.lib = dlopen("libaaudio.so", RTLD_NOW | RTLD_LOCAL);
	if (!api.lib)
		return -1;

	api.createStreamBuilder = (aaudio_result_t (*)(AAudioStreamBuilder **))load_symbol("AAudio_createStreamBuilder");
	api.deleteStreamBuilder = (void (*)(AAudioStreamBuilder *))load_symbol("AAudioStreamBuilder_delete");
	api.setDirection = (void (*)(AAudioStreamBuilder *, aaudio_direction_t))load_symbol("AAudioStreamBuilder_setDirection");
	api.setSampleRate = (void (*)(AAudioStreamBuilder *, int32_t))load_symbol("AAudioStreamBuilder_setSampleRate");
	api.setChannelCount = (void (*)(AAudioStreamBuilder *, int32_t))load_symbol("AAudioStreamBuilder_setChannelCount");
	api.setFormat = (void (*)(AAudioStreamBuilder *, aaudio_format_t))load_symbol("AAudioStreamBuilder_setFormat");
	api.setPerformanceMode = (void (*)(AAudioStreamBuilder *, aaudio_performance_mode_t))load_symbol("AAudioStreamBuilder_setPerformanceMode");
	api.setSharingMode = (void (*)(AAudioStreamBuilder *, aaudio_sharing_mode_t))load_symbol("AAudioStreamBuilder_setSharingMode");
	api.openStream = (aaudio_result_t (*)(AAudioStreamBuilder *, AAudioStream **))load_symbol("AAudioStreamBuilder_openStream");
	api.requestStart = (aaudio_result_t (*)(AAudioStream *))load_symbol("AAudioStream_requestStart");
	api.requestStop = (aaudio_result_t (*)(AAudioStream *))load_symbol("AAudioStream_requestStop");
	api.close = (aaudio_result_t (*)(AAudioStream *))load_symbol("AAudioStream_close");
	api.write = (aaudio_result_t (*)(AAudioStream *, const void *, int32_t, int64_t))load_symbol("AAudioStream_write");

	if (!api.createStreamBuilder || !api.deleteStreamBuilder || !api.setDirection ||
	    !api.setSampleRate || !api.setChannelCount || !api.setFormat ||
	    !api.setPerformanceMode || !api.setSharingMode || !api.openStream ||
	    !api.requestStart || !api.requestStop || !api.close || !api.write)
	{
		dlclose(api.lib);
		memset(&api, 0, sizeof(api));
		return -1;
	}

	return 0;
}

static int aaudio_init(int rate, int channels, int buffer, int flags)
{
	AAudioStreamBuilder *builder = NULL;
	aaudio_result_t result;

	(void)buffer;
	(void)flags;

	if (initialized)
		return 0;

	if (load_api() != 0)
		return -1;

	result = api.createStreamBuilder(&builder);
	if (result != AAUDIO_OK || !builder)
		return -1;

	api.setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
	api.setSampleRate(builder, rate > 0 ? rate : FB_SFX_DEFAULT_RATE);
	api.setChannelCount(builder, channels > 0 ? channels : FB_SFX_DEFAULT_CHANNELS);
	api.setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
	api.setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
	api.setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);

	result = api.openStream(builder, &stream);
	api.deleteStreamBuilder(builder);
	if (result != AAUDIO_OK || !stream)
	{
		stream = NULL;
		return -1;
	}

	result = api.requestStart(stream);
	if (result != AAUDIO_OK)
	{
		api.close(stream);
		stream = NULL;
		return -1;
	}

	initialized = 1;
	return 0;
}

static void aaudio_exit(void)
{
	if (stream)
	{
		api.requestStop(stream);
		api.close(stream);
		stream = NULL;
	}
	initialized = 0;
}

static int aaudio_write(const float *samples, int frames)
{
	aaudio_result_t result;

	if (!initialized || !stream || !samples || frames <= 0)
		return -1;

	result = api.write(stream, samples, frames, 100000000L);
	return result < 0 ? -1 : (int)result;
}

const FB_SFX_DRIVER fb_sfxDriverAAudio =
{
	"AAudio",
	0,
	aaudio_init,
	aaudio_exit,
	aaudio_write,
	NULL,
	NULL,
	NULL,
	NULL
};
