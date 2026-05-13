/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_capture_coreaudio.c

    Purpose:

        Provide the macOS CoreAudio input path for the generic
        sfxlib capture commands.

    Responsibilities:

        - open the default CoreAudio input device
        - receive interleaved 16-bit PCM frames from AudioQueue
        - feed those frames into the shared sfxlib capture ring buffer

    This file intentionally does NOT contain:

        - playback queue management
        - mixer logic
        - BASIC command parsing
*/

#ifndef DISABLE_DARWIN

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_darwin.h"

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FB_SFX_DARWIN_CAPTURE_BUFFERS 3

static AudioQueueRef g_capture_queue = NULL;
static AudioQueueBufferRef g_capture_buffers[FB_SFX_DARWIN_CAPTURE_BUFFERS];
static AudioStreamBasicDescription g_capture_format;
static int g_capture_buffer_bytes = 0;
static int g_capture_running = 0;

void fb_sfxPlatformCaptureStop(void);

static int fb_sfxDarwinCaptureDebugEnabled(void)
{
    const char *env = getenv("SFXLIB_DARWIN_DEBUG");
    return (env && *env && *env != '0');
}

#define CAPTURE_DBG(...) \
    do { if (fb_sfxDarwinCaptureDebugEnabled()) fprintf(stderr, "SFX_CAPTURE_DARWIN: " __VA_ARGS__); } while (0)

static void fb_sfxDarwinCaptureLogStatus(const char *where, OSStatus status)
{
    CAPTURE_DBG("%s failed (status=%ld)\n", where, (long)status);
}

static int fb_sfxDarwinHasInputDevice(void)
{
    AudioObjectPropertyAddress addr;
    AudioDeviceID device = kAudioObjectUnknown;
    UInt32 size = (UInt32)sizeof(device);
    OSStatus status;

    addr.mSelector = kAudioHardwarePropertyDefaultInputDevice;
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
        fb_sfxDarwinCaptureLogStatus("AudioObjectGetPropertyData(default input)", status);
        return 0;
    }

    return (device != kAudioObjectUnknown);
}

static int fb_sfxDarwinCaptureBufferFrames(void)
{
    int frames;

    frames = (fb_sfx_darwin.buffer_frames > 0)
        ? (fb_sfx_darwin.buffer_frames / 2)
        : (FB_SFX_DEFAULT_BUFFER / 2);

    if (frames < 512)
        frames = 512;
    else if (frames > 4096)
        frames = 4096;

    return frames;
}

static void fb_sfxDarwinCaptureCallback(void *user_data,
                                        AudioQueueRef queue,
                                        AudioQueueBufferRef buffer,
                                        const AudioTimeStamp *start_time,
                                        UInt32 packet_count,
                                        const AudioStreamPacketDescription *packet_descriptions)
{
    int frames;

    (void)user_data;
    (void)start_time;
    (void)packet_count;
    (void)packet_descriptions;

    if (!buffer || g_capture_format.mBytesPerFrame == 0)
        return;

    frames = (int)(buffer->mAudioDataByteSize / g_capture_format.mBytesPerFrame);
    if (g_capture_running && frames > 0)
    {
        if (!__fb_sfx || __fb_sfx->capture.enabled != FB_SFX_CAPTURE_PAUSED)
            fb_sfxCaptureWrite((const short *)buffer->mAudioData, frames);
    }

    if (g_capture_running)
        AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

int fb_sfxPlatformCaptureStart(void)
{
    OSStatus status;
    int channels;
    int rate;
    int frames;
    int i;

    if (g_capture_queue)
        return 0;

    if (!fb_sfxDarwinHasInputDevice())
        return -1;

    rate = (__fb_sfx && __fb_sfx->capture.rate > 0)
        ? __fb_sfx->capture.rate
        : FB_SFX_DEFAULT_RATE;
    channels = (__fb_sfx && __fb_sfx->capture.channels > 0)
        ? __fb_sfx->capture.channels
        : FB_SFX_DEFAULT_CHANNELS;

    memset(&g_capture_format, 0, sizeof(g_capture_format));
    g_capture_format.mSampleRate = (Float64)rate;
    g_capture_format.mFormatID = kAudioFormatLinearPCM;
    g_capture_format.mFormatFlags = kAudioFormatFlagIsSignedInteger |
                                    kAudioFormatFlagIsPacked |
                                    kAudioFormatFlagsNativeEndian;
    g_capture_format.mChannelsPerFrame = (UInt32)channels;
    g_capture_format.mBitsPerChannel = 16;
    g_capture_format.mFramesPerPacket = 1;
    g_capture_format.mBytesPerFrame = (UInt32)(sizeof(short) * channels);
    g_capture_format.mBytesPerPacket = g_capture_format.mBytesPerFrame;

    status = AudioQueueNewInput(&g_capture_format,
                                fb_sfxDarwinCaptureCallback,
                                NULL,
                                NULL,
                                NULL,
                                0,
                                &g_capture_queue);
    if (status != noErr)
    {
        fb_sfxDarwinCaptureLogStatus("AudioQueueNewInput", status);
        return -1;
    }

    frames = fb_sfxDarwinCaptureBufferFrames();
    g_capture_buffer_bytes = frames * (int)g_capture_format.mBytesPerFrame;
    fb_sfxCaptureBufferClear();

    for (i = 0; i < FB_SFX_DARWIN_CAPTURE_BUFFERS; ++i)
    {
        status = AudioQueueAllocateBuffer(g_capture_queue,
                                          (UInt32)g_capture_buffer_bytes,
                                          &g_capture_buffers[i]);
        if (status != noErr)
        {
            fb_sfxDarwinCaptureLogStatus("AudioQueueAllocateBuffer(capture)", status);
            fb_sfxPlatformCaptureStop();
            return -1;
        }

        status = AudioQueueEnqueueBuffer(g_capture_queue,
                                         g_capture_buffers[i],
                                         0,
                                         NULL);
        if (status != noErr)
        {
            fb_sfxDarwinCaptureLogStatus("AudioQueueEnqueueBuffer(capture)", status);
            fb_sfxPlatformCaptureStop();
            return -1;
        }
    }

    g_capture_running = 1;
    status = AudioQueueStart(g_capture_queue, NULL);
    if (status != noErr)
    {
        fb_sfxDarwinCaptureLogStatus("AudioQueueStart(capture)", status);
        fb_sfxPlatformCaptureStop();
        return -1;
    }

    CAPTURE_DBG("started (rate=%d channels=%d frames=%d)\n",
                rate,
                channels,
                frames);
    return 0;
}

void fb_sfxPlatformCaptureStop(void)
{
    int i;

    g_capture_running = 0;

    if (!g_capture_queue)
        return;

    AudioQueueStop(g_capture_queue, true);

    for (i = 0; i < FB_SFX_DARWIN_CAPTURE_BUFFERS; ++i)
    {
        if (g_capture_buffers[i])
        {
            AudioQueueFreeBuffer(g_capture_queue, g_capture_buffers[i]);
            g_capture_buffers[i] = NULL;
        }
    }

    AudioQueueDispose(g_capture_queue, true);
    g_capture_queue = NULL;
    g_capture_buffer_bytes = 0;
    memset(&g_capture_format, 0, sizeof(g_capture_format));

    CAPTURE_DBG("stopped\n");
}

int fb_sfxPlatformCaptureRead(float *buffer, int frames)
{
    if (!g_capture_queue)
        return 0;

    if (!buffer || frames <= 0)
        return -1;

    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.001, false);
    return 0;
}

#endif

/* end of sfx_capture_coreaudio.c */
