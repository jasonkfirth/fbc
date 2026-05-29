/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_asnd.c

    Purpose:

        Implement the Wii sfxlib playback driver using libogc's ASND
        audio interface.

    Responsibilities:

        - convert sfxlib float mixer samples to 16-bit stereo PCM
        - submit short PCM buffers to ASND voice 0
        - run a small libogc worker thread so BASIC code does not have
          to pump the mixer from the foreground
        - register the Wii driver list and null fallback

    This file intentionally does NOT contain:

        - file decoding
        - mixer logic
        - graphics or console lifecycle handling
        - emulator-specific behavior
*/

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"
#include "../fb_sfx_internal.h"

#include <asndlib.h>
#include <malloc.h>
#include <ogc/cache.h>
#include <ogc/lwp.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>


/* ------------------------------------------------------------------------- */
/* Wii audio constants                                                       */
/* ------------------------------------------------------------------------- */

/*
    ASND accepts signed 16-bit stereo PCM.  The Wii is a big-endian PowerPC
    target, so writing native signed short samples matches
    VOICE_STEREO_16BIT's big-endian sample layout.

    The buffer sizes are deliberately modest.  Larger buffers reduce the chance
    of underrun, but they also make short game sounds feel sluggish.  The
    worker keeps one ASND buffer in flight and one queued behind it.
*/

#define FB_SFX_WII_RATE          48000
#define FB_SFX_WII_CHANNELS      2
#define FB_SFX_WII_VOICE         0
#define FB_SFX_WII_BUFFER_COUNT  3
#define FB_SFX_WII_MIN_FRAMES    256
#define FB_SFX_WII_MAX_FRAMES    1024
#define FB_SFX_WII_STACK_SIZE    32768


/* ------------------------------------------------------------------------- */
/* Driver state                                                              */
/* ------------------------------------------------------------------------- */

static short *g_wii_buffers[FB_SFX_WII_BUFFER_COUNT];
static int g_wii_buffer_frames = 0;
static int g_wii_buffer_bytes = 0;
static int g_wii_buffer_alloc_bytes = 0;
static int g_wii_next_buffer = 0;
static int g_wii_initialized = 0;
static int g_wii_voice_started = 0;

static lwp_t g_wii_worker_thread = LWP_THREAD_NULL;
static unsigned char g_wii_worker_stack[FB_SFX_WII_STACK_SIZE] __attribute__((aligned(32)));
static volatile int g_wii_worker_stop = 0;


/* ------------------------------------------------------------------------- */
/* ASND callback                                                             */
/* ------------------------------------------------------------------------- */

/*
    ASND only treats the voice as a continuously refillable stream when
    ASND_SetVoice() is given a callback.  The worker still performs the actual
    refilling from normal thread context, but the callback must be present so
    ASND_AddVoice() can append the second buffer instead of rejecting it.
*/
static void wii_asnd_voice_callback(s32 voice)
{
    (void)voice;
}


/* ------------------------------------------------------------------------- */
/* Buffer helpers                                                            */
/* ------------------------------------------------------------------------- */

static int wii_align_32(int value)
{
    if (value <= 0)
        return 0;

    return (value + 31) & ~31;
}

static int wii_clamp_buffer_frames(int frames)
{
    if (frames < FB_SFX_WII_MIN_FRAMES)
        return FB_SFX_WII_MIN_FRAMES;

    if (frames > FB_SFX_WII_MAX_FRAMES)
        return FB_SFX_WII_MAX_FRAMES;

    return frames;
}

static void wii_clear_buffers(void)
{
    int i;

    for (i = 0; i < FB_SFX_WII_BUFFER_COUNT; ++i)
    {
        if (g_wii_buffers[i])
        {
            free(g_wii_buffers[i]);
            g_wii_buffers[i] = NULL;
        }
    }
}

static int wii_allocate_buffers(void)
{
    int i;

    for (i = 0; i < FB_SFX_WII_BUFFER_COUNT; ++i)
    {
        g_wii_buffers[i] = (short *)memalign(32, (size_t)g_wii_buffer_alloc_bytes);
        if (!g_wii_buffers[i])
        {
            wii_clear_buffers();
            return -1;
        }

        memset(g_wii_buffers[i], 0, (size_t)g_wii_buffer_alloc_bytes);
    }

    return 0;
}

static int wii_find_free_buffer(void)
{
    int attempts;
    int status;

    for (attempts = 0; attempts < 500; ++attempts)
    {
        int i;

        status = ASND_StatusVoice(FB_SFX_WII_VOICE);
        if (status == SND_UNUSED || status == SND_INVALID)
            return g_wii_next_buffer;

        for (i = 0; i < FB_SFX_WII_BUFFER_COUNT; ++i)
        {
            int index;

            index = (g_wii_next_buffer + i) % FB_SFX_WII_BUFFER_COUNT;
            if (ASND_TestPointer(FB_SFX_WII_VOICE, g_wii_buffers[index]) == 0)
                return index;
        }

        usleep(1000);
    }

    return -1;
}

static int wii_worker_frames(void)
{
    int frames;

    frames = g_wii_buffer_frames;

    if (frames <= 0)
        frames = FB_SFX_WII_MIN_FRAMES;

    if (frames < FB_SFX_WII_MIN_FRAMES)
        frames = FB_SFX_WII_MIN_FRAMES;
    else if (frames > FB_SFX_WII_MAX_FRAMES)
        frames = FB_SFX_WII_MAX_FRAMES;

    return frames;
}


/* ------------------------------------------------------------------------- */
/* Worker thread                                                             */
/* ------------------------------------------------------------------------- */

static void *wii_audio_worker(void *unused)
{
    (void)unused;

    while (!g_wii_worker_stop)
    {
        if (__fb_sfx && __fb_sfx->shutting_down)
        {
            usleep(5000);
            continue;
        }

        if (!g_wii_initialized)
        {
            usleep(5000);
            continue;
        }

        if (fb_sfxForegroundFeedActive())
        {
            usleep(5000);
            continue;
        }

        fb_sfxUpdate(wii_worker_frames());
        usleep(1000);
    }

    return NULL;
}

static int wii_ensure_worker(void)
{
    if (g_wii_worker_thread != LWP_THREAD_NULL)
        return 0;

    g_wii_worker_stop = 0;

    if (LWP_CreateThread(&g_wii_worker_thread,
                         wii_audio_worker,
                         NULL,
                         g_wii_worker_stack,
                         sizeof(g_wii_worker_stack),
                         80) != 0)
    {
        g_wii_worker_thread = LWP_THREAD_NULL;
        return -1;
    }

    return 0;
}

static void wii_stop_worker(void)
{
    lwp_t thread;

    if (g_wii_worker_thread == LWP_THREAD_NULL)
        return;

    thread = g_wii_worker_thread;
    g_wii_worker_thread = LWP_THREAD_NULL;
    g_wii_worker_stop = 1;

    if (thread != LWP_GetSelf())
        LWP_JoinThread(thread, NULL);
}


/* ------------------------------------------------------------------------- */
/* Driver lifecycle                                                          */
/* ------------------------------------------------------------------------- */

static int wii_driver_init(int rate, int channels, int buffer_frames, int flags)
{
    (void)rate;
    (void)channels;
    (void)flags;

    if (g_wii_initialized)
        return 0;

    g_wii_buffer_frames = wii_clamp_buffer_frames(buffer_frames);
    g_wii_buffer_bytes = g_wii_buffer_frames *
                         FB_SFX_WII_CHANNELS *
                         (int)sizeof(short);
    g_wii_buffer_alloc_bytes = wii_align_32(g_wii_buffer_bytes);
    g_wii_next_buffer = 0;
    g_wii_voice_started = 0;

    if (g_wii_buffer_alloc_bytes <= 0)
        return -1;

    if (wii_allocate_buffers() != 0)
        return -1;

    ASND_Init();
    ASND_Pause(0);

    if (__fb_sfx)
    {
        __fb_sfx->samplerate = FB_SFX_WII_RATE;
        __fb_sfx->output_channels = FB_SFX_WII_CHANNELS;
        __fb_sfx->buffer_size = g_wii_buffer_frames;
        __fb_sfx->buffer_frames = g_wii_buffer_frames;
    }

    g_wii_initialized = 1;

    if (wii_ensure_worker() != 0)
    {
        g_wii_initialized = 0;
        ASND_End();
        wii_clear_buffers();
        return -1;
    }

    return 0;
}

static void wii_driver_exit(void)
{
    if (!g_wii_initialized && g_wii_worker_thread == LWP_THREAD_NULL)
        return;

    wii_stop_worker();

    if (g_wii_initialized)
    {
        ASND_StopVoice(FB_SFX_WII_VOICE);
        ASND_Pause(1);
        ASND_End();
    }

    wii_clear_buffers();

    g_wii_buffer_frames = 0;
    g_wii_buffer_bytes = 0;
    g_wii_buffer_alloc_bytes = 0;
    g_wii_next_buffer = 0;
    g_wii_initialized = 0;
    g_wii_voice_started = 0;
}


/* ------------------------------------------------------------------------- */
/* Driver write                                                              */
/* ------------------------------------------------------------------------- */

static int wii_driver_submit(int index)
{
    int result;
    int status;
    int attempts;

    status = ASND_StatusVoice(FB_SFX_WII_VOICE);
    if (!g_wii_voice_started || status == SND_UNUSED || status == SND_INVALID)
    {
        result = ASND_SetVoice(FB_SFX_WII_VOICE,
                               VOICE_STEREO_16BIT,
                               FB_SFX_WII_RATE,
                               0,
                               g_wii_buffers[index],
                               g_wii_buffer_bytes,
                               255,
                               255,
                               wii_asnd_voice_callback);
        if (result == SND_OK)
        {
            g_wii_voice_started = 1;
            return 0;
        }

        g_wii_voice_started = 0;
        ASND_StopVoice(FB_SFX_WII_VOICE);
        return 1;
    }

    for (attempts = 0; attempts < 500; ++attempts)
    {
        if (ASND_TestVoiceBufferReady(FB_SFX_WII_VOICE) == 1)
        {
            result = ASND_AddVoice(FB_SFX_WII_VOICE,
                                   g_wii_buffers[index],
                                   g_wii_buffer_bytes);
            if (result == SND_OK)
                return 0;

            if (result != SND_BUSY)
            {
                g_wii_voice_started = 0;
                ASND_StopVoice(FB_SFX_WII_VOICE);
                return 1;
            }
        }

        usleep(1000);
    }

    return 1;
}

static int wii_driver_write_chunk(const float *samples, int frames)
{
    short *pcm;
    int index;
    int samples_to_copy;
    int submit_result;

    index = wii_find_free_buffer();
    if (index < 0)
        return frames;

    pcm = g_wii_buffers[index];
    samples_to_copy = frames * FB_SFX_WII_CHANNELS;

    fb_sfxConvertFloatToS16(samples, pcm, samples_to_copy);

    if (frames < g_wii_buffer_frames)
    {
        memset(pcm + samples_to_copy, 0,
               (size_t)(g_wii_buffer_frames - frames) *
               FB_SFX_WII_CHANNELS * sizeof(short));
    }

    /*
        ASND reads these buffers from the audio/DSP side.  The samples are
        written by the CPU into cached memory, so the cache must be flushed
        before the buffer is submitted or hardware can see stale data.
    */
    DCFlushRange(pcm, (u32)g_wii_buffer_alloc_bytes);

    submit_result = wii_driver_submit(index);
    if (submit_result < 0)
        return -1;

    if (submit_result > 0)
        return frames;

    g_wii_next_buffer = (index + 1) % FB_SFX_WII_BUFFER_COUNT;
    return frames;
}

static int wii_driver_write(const float *samples, int frames)
{
    int total;
    int channels;

    if (!g_wii_initialized || !samples || frames <= 0)
        return -1;

    channels = (__fb_sfx && __fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : FB_SFX_DEFAULT_CHANNELS;

    fb_sfxDriverDiagnostics("ASND", samples, frames, channels);

    total = 0;
    while (total < frames)
    {
        int chunk;
        int written;

        chunk = frames - total;
        if (chunk > g_wii_buffer_frames)
            chunk = g_wii_buffer_frames;

        written = wii_driver_write_chunk(samples + (total * FB_SFX_WII_CHANNELS),
                                         chunk);
        if (written <= 0)
            return total > 0 ? total : written;

        total += written;
    }

    return total;
}


/* ------------------------------------------------------------------------- */
/* Driver records                                                            */
/* ------------------------------------------------------------------------- */

static const FB_SFX_DRIVER fb_sfxDriverWiiAsnd =
{
    "asnd",
    FB_SFX_DRIVER_CAP_BACKGROUND,
    wii_driver_init,
    wii_driver_exit,
    wii_driver_write,
    NULL,
    NULL,
    NULL,
    NULL
};

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverWiiAsnd,
    &__fb_sfxDriverNull,
    NULL
};


/* end of sfx_driver_asnd.c */
