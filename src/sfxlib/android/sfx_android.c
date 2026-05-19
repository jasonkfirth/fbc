#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_android.h"

#include <stddef.h>
#include <pthread.h>
#include <time.h>

static pthread_mutex_t lifecycle_mutex = PTHREAD_MUTEX_INITIALIZER;
static int lifecycle_started = 1;
static int lifecycle_resumed = 1;

#if FB_SFX_MT_ENABLED
static pthread_t audio_thread;
static int audio_thread_valid = 0;
static volatile int audio_thread_stop = 0;
#endif

static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static int audio_running = 0;
static int audio_buffer_frames = FB_SFX_DEFAULT_BUFFER;

static void android_sfx_sleep_ms(unsigned long milliseconds)
{
	struct timespec req;

	req.tv_sec = (time_t)(milliseconds / 1000UL);
	req.tv_nsec = (long)((milliseconds % 1000UL) * 1000000UL);
	nanosleep(&req, NULL);
}

static int android_sfx_worker_frames(void)
{
	int frames;

	pthread_mutex_lock(&audio_mutex);
	frames = audio_buffer_frames;
	pthread_mutex_unlock(&audio_mutex);

	if (frames <= 0)
		frames = FB_SFX_DEFAULT_BUFFER;

	frames /= 4;

	if (frames < 256)
		frames = 256;
	else if (frames > 2048)
		frames = 2048;

	return frames;
}

#if FB_SFX_MT_ENABLED
static void *android_sfx_audio_worker(void *unused)
{
	(void)unused;

	while (!audio_thread_stop)
	{
		int running;

		pthread_mutex_lock(&audio_mutex);
		running = audio_running;
		pthread_mutex_unlock(&audio_mutex);

		if (!running || !fb_hAndroidSfxIsRunning())
		{
			android_sfx_sleep_ms(5);
			continue;
		}

		if (fb_sfxForegroundFeedActive())
		{
			android_sfx_sleep_ms(5);
			continue;
		}

		fb_sfxUpdate(android_sfx_worker_frames());
	}

	return NULL;
}

static int android_sfx_ensure_worker(void)
{
	if (audio_thread_valid)
		return 0;

	audio_thread_stop = 0;

	if (pthread_create(&audio_thread, NULL, android_sfx_audio_worker, NULL) != 0)
		return -1;

	audio_thread_valid = 1;
	return 0;
}
#endif

void fb_hAndroidSfxSetLifecycle(int started, int resumed)
{
	pthread_mutex_lock(&lifecycle_mutex);
	lifecycle_started = started ? 1 : 0;
	lifecycle_resumed = resumed ? 1 : 0;
	pthread_mutex_unlock(&lifecycle_mutex);
}

int fb_hAndroidSfxIsRunning(void)
{
	int running;

	pthread_mutex_lock(&lifecycle_mutex);
	running = lifecycle_started && lifecycle_resumed;
	pthread_mutex_unlock(&lifecycle_mutex);

	return running;
}

int fb_hAndroidSfxActivate(int rate, int channels, int buffer_frames)
{
	(void)rate;
	(void)channels;

	if (buffer_frames <= 0)
		buffer_frames = FB_SFX_DEFAULT_BUFFER;

#if FB_SFX_MT_ENABLED
	if (android_sfx_ensure_worker() != 0)
		return -1;
#endif

	pthread_mutex_lock(&audio_mutex);
	audio_buffer_frames = buffer_frames;
	audio_running = 1;
	pthread_mutex_unlock(&audio_mutex);

	return 0;
}

void fb_hAndroidSfxDeactivate(void)
{
	pthread_mutex_lock(&audio_mutex);
	audio_running = 0;
	pthread_mutex_unlock(&audio_mutex);
}

void fb_hAndroidSfxExit(void)
{
	fb_hAndroidSfxDeactivate();

#if FB_SFX_MT_ENABLED
	if (audio_thread_valid)
	{
		audio_thread_stop = 1;
		if (!pthread_equal(audio_thread, pthread_self()))
			pthread_join(audio_thread, NULL);
		audio_thread_valid = 0;
	}
#endif
}

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
	&fb_sfxDriverAAudio,
	&fb_sfxDriverOpenSLES,
	&__fb_sfxDriverNull,
	NULL
};
