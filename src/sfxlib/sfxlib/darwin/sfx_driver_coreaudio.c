/*
    Basic CoreAudio playback driver for macOS.
*/

#ifndef DISABLE_DARWIN

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_darwin.h"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FB_SFX_DARWIN_QUEUE_BUFFERS 3

typedef struct FB_SFX_DARWIN_QUEUE_BUFFER
{
    AudioQueueBufferRef ref;
    int in_use;
} FB_SFX_DARWIN_QUEUE_BUFFER;

FB_SFX_DARWIN_STATE fb_sfx_darwin =
{
    0,
    FB_SFX_DEFAULT_RATE,
    FB_SFX_DEFAULT_CHANNELS,
    FB_SFX_DEFAULT_BUFFER,
    0,
    NULL
};

static AudioQueueRef g_audio_queue = NULL;
static AudioStreamBasicDescription g_format;
static FB_SFX_DARWIN_QUEUE_BUFFER g_buffers[FB_SFX_DARWIN_QUEUE_BUFFERS];
static int g_buffer_bytes = 0;
static int g_current_buffer = 0;
static int g_write_debug_count = 0;
static int g_done_debug_count = 0;
static int g_darwin_debug_initialized = 0;
static int g_darwin_debug_enabled = 0;

#if FB_SFX_MT_ENABLED
static pthread_t g_audio_thread;
static int g_audio_thread_valid = 0;
static volatile int g_audio_thread_stop = 0;
#endif

static void fb_sfxDarwinInitDebug(void)
{
    const char *env;

    if (g_darwin_debug_initialized)
        return;

    g_darwin_debug_initialized = 1;
    env = getenv("SFXLIB_DARWIN_DEBUG");
    g_darwin_debug_enabled = (env && *env && *env != '0');
}

static int fb_sfxDarwinDebugEnabled(void)
{
    fb_sfxDarwinInitDebug();
    return g_darwin_debug_enabled;
}

#define DARWIN_DBG(...) \
    do { if (fb_sfxDarwinDebugEnabled()) fprintf(stderr, "SFX_DARWIN: " __VA_ARGS__); } while (0)

static void fb_sfxDarwinLogStatus(const char *where, OSStatus status)
{
    DARWIN_DBG("%s failed (status=%ld)\n", where, (long)status);
}

static int fb_sfxDarwinHasOutputDevice(void)
{
    AudioObjectPropertyAddress addr;
    AudioDeviceID device = kAudioObjectUnknown;
    UInt32 size = (UInt32)sizeof(device);
    OSStatus status;

    addr.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    addr.mScope = kAudioObjectPropertyScopeGlobal;
    addr.mElement = kAudioObjectPropertyElementMain;

    status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                        &addr,
                                        0,
                                        NULL,
                                        &size,
                                        &device);
    if (status != noErr)
    {
        fb_sfxDarwinLogStatus("AudioObjectGetPropertyData(default output)", status);
        return 0;
    }

    if (device == kAudioObjectUnknown)
    {
        DARWIN_DBG("no default output device is available\n");
        return 0;
    }

    DARWIN_DBG("default output device id=%u\n", (unsigned)device);
    return 1;
}

static void fb_sfxDarwinSleepMs(unsigned long milliseconds)
{
    struct timespec req;

    req.tv_sec = (time_t)(milliseconds / 1000UL);
    req.tv_nsec = (long)((milliseconds % 1000UL) * 1000000UL);
    nanosleep(&req, NULL);
}

static int fb_sfxDarwinWorkerFrames(void)
{
    int frames;

    frames = (fb_sfx_darwin.buffer_frames > 0)
        ? (fb_sfx_darwin.buffer_frames / 4)
        : (FB_SFX_DEFAULT_BUFFER / 4);

    if (frames < 256)
        frames = 256;
    else if (frames > 2048)
        frames = 2048;

    return frames;
}

#if FB_SFX_MT_ENABLED
static void *fb_sfxDarwinAudioWorker(void *unused)
{
    (void)unused;

    while (!g_audio_thread_stop)
    {
        if (!fb_sfx_darwin.running)
        {
            fb_sfxDarwinSleepMs(5);
            continue;
        }

        fb_sfxUpdate(fb_sfxDarwinWorkerFrames());
    }

    return NULL;
}

static int fb_sfxDarwinEnsureWorker(void)
{
    if (g_audio_thread_valid)
        return 0;

    g_audio_thread_stop = 0;

    if (pthread_create(&g_audio_thread, NULL, fb_sfxDarwinAudioWorker, NULL) != 0)
        return -1;

    g_audio_thread_valid = 1;
    return 0;
}
#endif

static void fb_sfxDarwinBufferDone(void *user_data,
                                   AudioQueueRef queue,
                                   AudioQueueBufferRef buffer)
{
    FB_SFX_DARWIN_QUEUE_BUFFER *buffers = (FB_SFX_DARWIN_QUEUE_BUFFER *)user_data;
    int i;

    (void)queue;

    for (i = 0; i < FB_SFX_DARWIN_QUEUE_BUFFERS; ++i)
    {
        if (buffers[i].ref == buffer)
        {
            buffers[i].in_use = 0;
            if (g_done_debug_count < 8)
            {
                DARWIN_DBG("buffer done: slot=%d bytes=%u\n",
                           i,
                           (unsigned)buffer->mAudioDataByteSize);
                g_done_debug_count++;
            }
            break;
        }
    }
}

int fb_sfxDarwinInit(void)
{
    OSStatus status;
    UInt32 i;

    fb_sfxDarwinInitDebug();

    if (fb_sfx_darwin.initialized)
        return 0;

    if (!fb_sfxDarwinHasOutputDevice())
        return -1;

    memset(&g_format, 0, sizeof(g_format));
    g_format.mSampleRate = (Float64)fb_sfx_darwin.sample_rate;
    g_format.mFormatID = kAudioFormatLinearPCM;
    g_format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    g_format.mChannelsPerFrame = (UInt32)fb_sfx_darwin.channels;
    g_format.mBitsPerChannel = 32;
    g_format.mFramesPerPacket = 1;
    g_format.mBytesPerFrame = (UInt32)(sizeof(float) * fb_sfx_darwin.channels);
    g_format.mBytesPerPacket = g_format.mBytesPerFrame;

    status = AudioQueueNewOutput(&g_format,
                                 fb_sfxDarwinBufferDone,
                                 g_buffers,
                                 NULL,
                                 NULL,
                                 0,
                                 &g_audio_queue);
    if (status != noErr)
    {
        fb_sfxDarwinLogStatus("AudioQueueNewOutput", status);
        return -1;
    }

    g_buffer_bytes = fb_sfx_darwin.buffer_frames * (int)g_format.mBytesPerFrame;
    if (g_buffer_bytes <= 0)
        g_buffer_bytes = FB_SFX_DEFAULT_BUFFER * (int)g_format.mBytesPerFrame;

    for (i = 0; i < FB_SFX_DARWIN_QUEUE_BUFFERS; ++i)
    {
        status = AudioQueueAllocateBuffer(g_audio_queue,
                                          (UInt32)g_buffer_bytes,
                                          &g_buffers[i].ref);
        if (status != noErr)
        {
            fb_sfxDarwinLogStatus("AudioQueueAllocateBuffer", status);
            fb_sfxDarwinExit();
            return -1;
        }

        memset(g_buffers[i].ref->mAudioData, 0, (size_t)g_buffer_bytes);
        g_buffers[i].ref->mAudioDataByteSize = (UInt32)g_buffer_bytes;
        g_buffers[i].in_use = 1;

        status = AudioQueueEnqueueBuffer(g_audio_queue, g_buffers[i].ref, 0, NULL);
        if (status != noErr)
        {
            fb_sfxDarwinLogStatus("AudioQueueEnqueueBuffer(init)", status);
            g_buffers[i].in_use = 0;
            fb_sfxDarwinExit();
            return -1;
        }
    }

    g_current_buffer = 0;

    status = AudioQueueStart(g_audio_queue, NULL);
    if (status != noErr)
    {
        fb_sfxDarwinLogStatus("AudioQueueStart", status);
        fb_sfxDarwinExit();
        return -1;
    }

    fb_sfx_darwin.initialized = 1;
    fb_sfx_darwin.device_handle = g_audio_queue;

    DARWIN_DBG("initialized (rate=%d channels=%d buffer=%d)\n",
               fb_sfx_darwin.sample_rate,
               fb_sfx_darwin.channels,
               fb_sfx_darwin.buffer_frames);

    return 0;
}

int fb_sfxDarwinActivate(int rate, int channels, int buffer_frames)
{
    fb_sfx_darwin.sample_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    fb_sfx_darwin.channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;
    fb_sfx_darwin.buffer_frames = (buffer_frames > 0) ? buffer_frames : FB_SFX_DEFAULT_BUFFER;

    if (fb_sfx_darwin.initialized)
    {
        if ((int)g_format.mSampleRate != fb_sfx_darwin.sample_rate ||
            (int)g_format.mChannelsPerFrame != fb_sfx_darwin.channels ||
            g_buffer_bytes != fb_sfx_darwin.buffer_frames * (int)(sizeof(float) * fb_sfx_darwin.channels))
        {
            DARWIN_DBG("format change requested, rebuilding queue\n");
            fb_sfxDarwinExit();
        }
    }

    if (fb_sfxDarwinInit() != 0)
        return -1;

#if FB_SFX_MT_ENABLED
    if (fb_sfxDarwinEnsureWorker() != 0)
        return -1;
#endif

    fb_sfx_darwin.running = 1;
    return 0;
}

void fb_sfxDarwinDeactivate(void)
{
    fb_sfx_darwin.running = 0;
}

void fb_sfxDarwinExit(void)
{
    int i;

    fb_sfx_darwin.running = 0;

#if FB_SFX_MT_ENABLED
    if (g_audio_thread_valid)
    {
        g_audio_thread_stop = 1;
        if (!pthread_equal(g_audio_thread, pthread_self()))
            pthread_join(g_audio_thread, NULL);
        g_audio_thread_valid = 0;
    }
#endif

    if (g_audio_queue)
    {
        AudioQueueStop(g_audio_queue, true);

        for (i = 0; i < FB_SFX_DARWIN_QUEUE_BUFFERS; ++i)
        {
            if (g_buffers[i].ref)
            {
                AudioQueueFreeBuffer(g_audio_queue, g_buffers[i].ref);
                g_buffers[i].ref = NULL;
                g_buffers[i].in_use = 0;
            }
        }

        AudioQueueDispose(g_audio_queue, true);
        g_audio_queue = NULL;
    }

    fb_sfx_darwin.initialized = 0;
    fb_sfx_darwin.device_handle = NULL;
    g_current_buffer = 0;

    DARWIN_DBG("shutdown\n");
}

int fb_sfxDarwinWrite(float *buffer, int frames)
{
    FB_SFX_DARWIN_QUEUE_BUFFER *slot;
    size_t bytes;

    if (!g_audio_queue || !buffer || frames <= 0)
        return -1;

    slot = &g_buffers[g_current_buffer];

    while (slot->in_use)
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.001, false);

    bytes = (size_t)frames * (size_t)g_format.mBytesPerFrame;
    if (bytes > (size_t)g_buffer_bytes)
        bytes = (size_t)g_buffer_bytes;

    memcpy(slot->ref->mAudioData, buffer, bytes);
    slot->ref->mAudioDataByteSize = (UInt32)bytes;
    slot->in_use = 1;

    if (AudioQueueEnqueueBuffer(g_audio_queue, slot->ref, 0, NULL) != noErr)
    {
        slot->in_use = 0;
        return -1;
    }

    if (g_write_debug_count < 8)
    {
        DARWIN_DBG("enqueue: slot=%d frames=%d bytes=%zu\n",
                   g_current_buffer,
                   frames,
                   bytes);
        g_write_debug_count++;
    }

    g_current_buffer = (g_current_buffer + 1) % FB_SFX_DARWIN_QUEUE_BUFFERS;
    return (int)(bytes / g_format.mBytesPerFrame);
}

int fb_sfxDarwinRunning(void)
{
    return fb_sfx_darwin.running;
}

static int darwin_init(int rate, int channels, int buffer, int flags)
{
    (void)flags;

    fb_sfx_darwin.sample_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    fb_sfx_darwin.channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;
    fb_sfx_darwin.buffer_frames = (buffer > 0) ? buffer : FB_SFX_DEFAULT_BUFFER;

    return fb_sfxDarwinActivate(fb_sfx_darwin.sample_rate,
                                fb_sfx_darwin.channels,
                                fb_sfx_darwin.buffer_frames);
}

static void darwin_exit(void)
{
    fb_sfxDarwinDeactivate();
    fb_sfxDarwinExit();
}

static int darwin_write(const float *samples, int frames)
{
    return fb_sfxDarwinWrite((float *)samples, frames);
}

const FB_SFX_DRIVER fb_sfxDriverCoreAudio =
{
    "CoreAudio",
    0,
    darwin_init,
    darwin_exit,
    darwin_write,
    NULL,
    NULL,
    NULL,
    NULL
};

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverCoreAudio,
    NULL
};

#endif

/* end of sfx_driver_coreaudio.c */
