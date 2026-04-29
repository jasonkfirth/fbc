#include "../fb_sfx_driver.h"
#include "fb_sfx_android.h"

#include <stddef.h>
#include <pthread.h>

extern const FB_SFX_DRIVER __fb_sfxDriverNull;

static pthread_mutex_t lifecycle_mutex = PTHREAD_MUTEX_INITIALIZER;
static int lifecycle_started = 1;
static int lifecycle_resumed = 1;

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

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
	&fb_sfxDriverAAudio,
	&fb_sfxDriverOpenSLES,
	&__fb_sfxDriverNull,
	NULL
};
