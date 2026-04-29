#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_android.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FB_SFX_OPENSL_BUFFERS 3

typedef struct FB_SFX_OPENSL_BUFFER
{
	int16_t *samples;
	int bytes;
	int in_use;
} FB_SFX_OPENSL_BUFFER;

static SLObjectItf engine_object = NULL;
static SLEngineItf engine = NULL;
static SLObjectItf output_mix = NULL;
static SLObjectItf player_object = NULL;
static SLPlayItf player = NULL;
static SLAndroidSimpleBufferQueueItf queue = NULL;
static FB_SFX_OPENSL_BUFFER buffers[FB_SFX_OPENSL_BUFFERS];
static int channels_active = FB_SFX_DEFAULT_CHANNELS;
static int buffer_frames_active = FB_SFX_DEFAULT_BUFFER;
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t buffer_cond = PTHREAD_COND_INITIALIZER;
static int initialized = 0;
static int shutting_down = 0;

static void opensl_callback(SLAndroidSimpleBufferQueueItf caller, void *context)
{
	int i;

	(void)caller;
	(void)context;

	pthread_mutex_lock(&buffer_mutex);
	for (i = 0; i < FB_SFX_OPENSL_BUFFERS; ++i)
	{
		if (buffers[i].in_use)
		{
			buffers[i].in_use = 0;
			break;
		}
	}
	pthread_cond_signal(&buffer_cond);
	pthread_mutex_unlock(&buffer_mutex);
}

static void release_all_buffers_locked(void)
{
	int i;

	for (i = 0; i < FB_SFX_OPENSL_BUFFERS; ++i)
		buffers[i].in_use = 0;
	pthread_cond_broadcast(&buffer_cond);
}

static void free_buffers(void)
{
	int i;

	for (i = 0; i < FB_SFX_OPENSL_BUFFERS; ++i)
	{
		free(buffers[i].samples);
		buffers[i].samples = NULL;
		buffers[i].bytes = 0;
		buffers[i].in_use = 0;
	}
}

static int allocate_buffers(int frames, int channels)
{
	int i;
	int bytes;

	if (frames <= 0)
		frames = FB_SFX_DEFAULT_BUFFER;
	if (channels <= 0)
		channels = FB_SFX_DEFAULT_CHANNELS;

	bytes = frames * channels * (int)sizeof(int16_t);
	for (i = 0; i < FB_SFX_OPENSL_BUFFERS; ++i)
	{
		buffers[i].samples = (int16_t *)malloc((size_t)bytes);
		if (!buffers[i].samples)
		{
			free_buffers();
			return -1;
		}
		buffers[i].bytes = bytes;
		buffers[i].in_use = 0;
	}

	buffer_frames_active = frames;
	channels_active = channels;
	return 0;
}

static SLuint32 channel_mask_for(int channels)
{
	return channels == 1 ? SL_SPEAKER_FRONT_CENTER :
		(SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
}

static SLuint32 millihz_for(int rate)
{
	return (SLuint32)((rate > 0 ? rate : FB_SFX_DEFAULT_RATE) * 1000);
}

static void make_buffer_wait_timeout(struct timespec *ts)
{
	clock_gettime(CLOCK_REALTIME, ts);
	ts->tv_nsec += 50000000L;
	if (ts->tv_nsec >= 1000000000L)
	{
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000L;
	}
}

static int opensl_init(int rate, int channels, int buffer, int flags)
{
	SLresult result;
	SLDataLocator_AndroidSimpleBufferQueue loc_queue;
	SLDataFormat_PCM format_pcm;
	SLDataSource audio_source;
	SLDataLocator_OutputMix loc_outmix;
	SLDataSink audio_sink;
	const SLInterfaceID ids[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
	const SLboolean req[1] = {SL_BOOLEAN_TRUE};

	(void)flags;

	if (initialized)
		return 0;

	if (channels != 1 && channels != 2)
		channels = FB_SFX_DEFAULT_CHANNELS;

	pthread_mutex_lock(&buffer_mutex);
	shutting_down = 0;
	pthread_mutex_unlock(&buffer_mutex);

	if (allocate_buffers(buffer, channels) != 0)
		return -1;

	result = slCreateEngine(&engine_object, 0, NULL, 0, NULL, NULL);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	result = (*engine_object)->Realize(engine_object, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	result = (*engine_object)->GetInterface(engine_object, SL_IID_ENGINE, &engine);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	result = (*engine)->CreateOutputMix(engine, &output_mix, 0, NULL, NULL);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	result = (*output_mix)->Realize(output_mix, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	loc_queue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	loc_queue.numBuffers = FB_SFX_OPENSL_BUFFERS;

	format_pcm.formatType = SL_DATAFORMAT_PCM;
	format_pcm.numChannels = (SLuint32)channels;
	format_pcm.samplesPerSec = millihz_for(rate);
	format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
	format_pcm.channelMask = channel_mask_for(channels);
	format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

	audio_source.pLocator = &loc_queue;
	audio_source.pFormat = &format_pcm;

	loc_outmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	loc_outmix.outputMix = output_mix;
	audio_sink.pLocator = &loc_outmix;
	audio_sink.pFormat = NULL;

	result = (*engine)->CreateAudioPlayer(engine, &player_object, &audio_source, &audio_sink, 1, ids, req);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	result = (*player_object)->Realize(player_object, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	result = (*player_object)->GetInterface(player_object, SL_IID_PLAY, &player);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	result = (*player_object)->GetInterface(player_object, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &queue);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	result = (*queue)->RegisterCallback(queue, opensl_callback, NULL);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	result = (*player)->SetPlayState(player, SL_PLAYSTATE_PLAYING);
	if (result != SL_RESULT_SUCCESS)
		goto fail;

	initialized = 1;
	return 0;

fail:
	if (player_object)
	{
		(*player_object)->Destroy(player_object);
		player_object = NULL;
		player = NULL;
		queue = NULL;
	}
	if (output_mix)
	{
		(*output_mix)->Destroy(output_mix);
		output_mix = NULL;
	}
	if (engine_object)
	{
		(*engine_object)->Destroy(engine_object);
		engine_object = NULL;
		engine = NULL;
	}
	free_buffers();
	return -1;
}

static void opensl_exit(void)
{
	pthread_mutex_lock(&buffer_mutex);
	shutting_down = 1;
	initialized = 0;
	release_all_buffers_locked();
	pthread_mutex_unlock(&buffer_mutex);

	if (player_object)
	{
		(*player_object)->Destroy(player_object);
		player_object = NULL;
		player = NULL;
		queue = NULL;
	}
	if (output_mix)
	{
		(*output_mix)->Destroy(output_mix);
		output_mix = NULL;
	}
	if (engine_object)
	{
		(*engine_object)->Destroy(engine_object);
		engine_object = NULL;
		engine = NULL;
	}
	free_buffers();
}

static FB_SFX_OPENSL_BUFFER *wait_for_buffer(int *paused)
{
	int i;

	*paused = 0;
	pthread_mutex_lock(&buffer_mutex);
	while (!shutting_down)
	{
		struct timespec timeout;

		if (!fb_hAndroidSfxIsRunning())
		{
			*paused = 1;
			pthread_mutex_unlock(&buffer_mutex);
			return NULL;
		}

		for (i = 0; i < FB_SFX_OPENSL_BUFFERS; ++i)
		{
			if (!buffers[i].in_use)
			{
				buffers[i].in_use = 1;
				pthread_mutex_unlock(&buffer_mutex);
				return &buffers[i];
			}
		}
		make_buffer_wait_timeout(&timeout);
		pthread_cond_timedwait(&buffer_cond, &buffer_mutex, &timeout);
	}
	pthread_mutex_unlock(&buffer_mutex);
	return NULL;
}

static int opensl_write(const float *samples, int frames)
{
	FB_SFX_OPENSL_BUFFER *buffer;
	int total, i;
	int paused = 0;
	SLresult result;

	if (!fb_hAndroidSfxIsRunning())
		return 0;

	if (!initialized || !queue || !samples || frames <= 0)
		return -1;

	if (frames > buffer_frames_active)
		frames = buffer_frames_active;

	buffer = wait_for_buffer(&paused);
	if (!buffer)
		return paused ? 0 : -1;

	total = frames * channels_active;
	for (i = 0; i < total; ++i)
	{
		float s = samples[i];
		if (s > 1.0f)
			s = 1.0f;
		else if (s < -1.0f)
			s = -1.0f;
		buffer->samples[i] = (int16_t)(s * 32767.0f);
	}

	result = (*queue)->Enqueue(queue, buffer->samples, (SLuint32)(total * (int)sizeof(int16_t)));
	if (result != SL_RESULT_SUCCESS)
	{
		pthread_mutex_lock(&buffer_mutex);
		buffer->in_use = 0;
		pthread_cond_signal(&buffer_cond);
		pthread_mutex_unlock(&buffer_mutex);
		return -1;
	}

	return frames;
}

const FB_SFX_DRIVER fb_sfxDriverOpenSLES =
{
	"OpenSL ES",
	0,
	opensl_init,
	opensl_exit,
	opensl_write,
	NULL,
	NULL,
	NULL,
	NULL
};
