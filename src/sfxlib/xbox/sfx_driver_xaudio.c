/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_xaudio.c

    Purpose:

        Implement the Original Xbox sfxlib playback driver using nxdk's
        XAudio AC97 backend.

    Responsibilities:

        - convert sfxlib float mixer samples to 16-bit stereo PCM
        - feed contiguous DMA buffers to nxdk XAudio
        - register the Xbox driver list and null fallback
        - provide no-op MIDI driver hooks for targets without MIDI output

    This file intentionally does NOT contain:

        - file decoding
        - mixer logic
        - gfxlib or console lifecycle handling
        - emulator-specific code
*/

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"
#include "../fb_sfx_internal.h"

#include <hal/audio.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#include <stddef.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Xbox audio constants                                                      */
/* ------------------------------------------------------------------------- */

/*
    nxdk's XAudio wrapper currently feeds the Xbox AC97 hardware as
    signed 16-bit stereo PCM.  The public XAudioInit() signature accepts
    format parameters, but nxdk's implementation documents those as ignored
    for now.  sfxlib therefore asks its mixer for the same format the
    hardware path consumes so tones play at the intended pitch.
*/

#define FB_SFX_XBOX_RATE          48000
#define FB_SFX_XBOX_CHANNELS      2
#define FB_SFX_XBOX_BUFFER_COUNT  32
#define FB_SFX_XBOX_MIN_FRAMES    512
#define FB_SFX_XBOX_MAX_FRAMES    2048
#define FB_SFX_XBOX_START_BUFFERS 4

/*
    The nxdk xaudio sample uses this upper allocation bound for contiguous
    audio buffers.  The AC97 DMA engine needs physical memory, so malloc()
    is not suitable for the buffers handed to XAudioProvideSamples().
*/

#define FB_SFX_XBOX_MAXRAM        0x03FFAFFF


/* ------------------------------------------------------------------------- */
/* Driver state                                                              */
/* ------------------------------------------------------------------------- */

static unsigned char *g_xbox_buffers[FB_SFX_XBOX_BUFFER_COUNT];
static HANDLE g_xbox_free_buffers = NULL;
static int g_xbox_buffer_frames = 0;
static int g_xbox_buffer_bytes = 0;
static int g_xbox_next_buffer = 0;
static int g_xbox_initialized = 0;
static int g_xbox_playing = 0;
static int g_xbox_queued_before_play = 0;


/* ------------------------------------------------------------------------- */
/* Buffer helpers                                                            */
/* ------------------------------------------------------------------------- */

static int xbox_clamp_buffer_frames(int frames)
{
    if (frames < FB_SFX_XBOX_MIN_FRAMES)
        return FB_SFX_XBOX_MIN_FRAMES;

    if (frames > FB_SFX_XBOX_MAX_FRAMES)
        return FB_SFX_XBOX_MAX_FRAMES;

    return frames;
}

static short xbox_float_to_pcm16(float sample)
{
    if (sample >= 1.0f)
        return 32767;

    if (sample <= -1.0f)
        return -32768;

    if (sample >= 0.0f)
        return (short)(sample * 32767.0f);

    return (short)(sample * 32768.0f);
}

static void xbox_clear_buffers(void)
{
    int i;

    for (i = 0; i < FB_SFX_XBOX_BUFFER_COUNT; ++i)
    {
        if (g_xbox_buffers[i])
        {
            MmFreeContiguousMemory(g_xbox_buffers[i]);
            g_xbox_buffers[i] = NULL;
        }
    }
}

static int xbox_allocate_buffers(void)
{
    int i;

    for (i = 0; i < FB_SFX_XBOX_BUFFER_COUNT; ++i)
    {
        g_xbox_buffers[i] = MmAllocateContiguousMemoryEx(
            (SIZE_T)g_xbox_buffer_bytes,
            0,
            FB_SFX_XBOX_MAXRAM,
            0,
            PAGE_READWRITE | PAGE_WRITECOMBINE);

        if (!g_xbox_buffers[i])
        {
            xbox_clear_buffers();
            return -1;
        }

        memset(g_xbox_buffers[i], 0, (size_t)g_xbox_buffer_bytes);
    }

    return 0;
}

static int xbox_take_free_buffer(int timeout_ms)
{
    DWORD wait_result;

    wait_result = WaitForSingleObject(g_xbox_free_buffers, (DWORD)timeout_ms);
    return (wait_result == WAIT_OBJECT_0) ? 0 : -1;
}

static int xbox_queue_silence_buffer(void)
{
    int index;

    if (xbox_take_free_buffer(0) != 0)
        return -1;

    index = g_xbox_next_buffer;
    g_xbox_next_buffer = (g_xbox_next_buffer + 1) % FB_SFX_XBOX_BUFFER_COUNT;

    memset(g_xbox_buffers[index], 0, (size_t)g_xbox_buffer_bytes);
    XAudioProvideSamples(g_xbox_buffers[index],
                         (unsigned short)g_xbox_buffer_bytes,
                         FALSE);

    if (!g_xbox_playing)
        g_xbox_queued_before_play++;

    return 0;
}

static void xbox_start_playback_if_ready(void)
{
    if (g_xbox_playing)
        return;

    if (g_xbox_queued_before_play < FB_SFX_XBOX_START_BUFFERS)
        return;

    XAudioPlay();
    g_xbox_playing = 1;
}


/* ------------------------------------------------------------------------- */
/* XAudio callback                                                           */
/* ------------------------------------------------------------------------- */

/*
    nxdk calls this from a DPC.  Keep it intentionally tiny: no floating point
    work, no allocation, and no sfxlib locks.  It only releases one buffer
    token so the normal driver write path can queue another DMA buffer.
*/

static void xbox_audio_callback(void *pac97_device, void *data)
{
    (void)pac97_device;
    (void)data;

    if (g_xbox_free_buffers)
        ReleaseSemaphore(g_xbox_free_buffers, 1, NULL);
}


/* ------------------------------------------------------------------------- */
/* Driver lifecycle                                                          */
/* ------------------------------------------------------------------------- */

static int xbox_driver_init(int rate, int channels, int buffer_frames, int flags)
{
    (void)rate;
    (void)channels;
    (void)flags;

    if (g_xbox_initialized)
        return 0;

    g_xbox_buffer_frames = xbox_clamp_buffer_frames(buffer_frames);
    g_xbox_buffer_bytes = g_xbox_buffer_frames * FB_SFX_XBOX_CHANNELS * (int)sizeof(short);
    g_xbox_next_buffer = 0;
    g_xbox_queued_before_play = 0;

    g_xbox_free_buffers = CreateSemaphore(NULL, FB_SFX_XBOX_BUFFER_COUNT,
                                          FB_SFX_XBOX_BUFFER_COUNT, NULL);
    if (!g_xbox_free_buffers)
        return -1;

    if (xbox_allocate_buffers() != 0)
    {
        CloseHandle(g_xbox_free_buffers);
        g_xbox_free_buffers = NULL;
        return -1;
    }

    XAudioInit(16, FB_SFX_XBOX_CHANNELS, xbox_audio_callback, NULL);

    if (__fb_sfx)
    {
        __fb_sfx->samplerate = FB_SFX_XBOX_RATE;
        __fb_sfx->output_channels = FB_SFX_XBOX_CHANNELS;
        __fb_sfx->buffer_size = g_xbox_buffer_frames;
        __fb_sfx->buffer_frames = g_xbox_buffer_frames;
    }

    g_xbox_initialized = 1;
    g_xbox_playing = 0;
    return 0;
}

static void xbox_driver_exit(void)
{
    if (!g_xbox_initialized && !g_xbox_free_buffers)
        return;

    XAudioPause();
    XAudioInit(16, FB_SFX_XBOX_CHANNELS, NULL, NULL);

    xbox_clear_buffers();

    if (g_xbox_free_buffers)
    {
        CloseHandle(g_xbox_free_buffers);
        g_xbox_free_buffers = NULL;
    }

    g_xbox_buffer_frames = 0;
    g_xbox_buffer_bytes = 0;
    g_xbox_next_buffer = 0;
    g_xbox_initialized = 0;
    g_xbox_playing = 0;
    g_xbox_queued_before_play = 0;
}


/* ------------------------------------------------------------------------- */
/* Driver write                                                              */
/* ------------------------------------------------------------------------- */

static int xbox_driver_write_chunk(const float *samples, int frames)
{
    short *pcm;
    int i;
    int index;

    if (xbox_take_free_buffer(1000) != 0)
        return -1;

    index = g_xbox_next_buffer;
    g_xbox_next_buffer = (g_xbox_next_buffer + 1) % FB_SFX_XBOX_BUFFER_COUNT;

    pcm = (short *)g_xbox_buffers[index];
    for (i = 0; i < frames * FB_SFX_XBOX_CHANNELS; ++i)
        pcm[i] = xbox_float_to_pcm16(samples[i]);

    if (frames < g_xbox_buffer_frames)
    {
        memset(pcm + (frames * FB_SFX_XBOX_CHANNELS), 0,
               (size_t)(g_xbox_buffer_frames - frames) *
               FB_SFX_XBOX_CHANNELS * sizeof(short));
    }

    XAudioProvideSamples(g_xbox_buffers[index],
                         (unsigned short)g_xbox_buffer_bytes,
                         FALSE);

    if (!g_xbox_playing)
        g_xbox_queued_before_play++;

    /*
        nxdk's AC97 path expects active descriptors before playback starts.
        Queue at least two descriptors so the hardware has a real range to
        consume.  Very short first writes get one silent descriptor rather
        than falling back to the empty-ring startup path.
    */
    if (!g_xbox_playing &&
        g_xbox_queued_before_play < FB_SFX_XBOX_START_BUFFERS &&
        frames < g_xbox_buffer_frames)
        (void)xbox_queue_silence_buffer();

    xbox_start_playback_if_ready();

    return frames;
}

static int xbox_driver_write(const float *samples, int frames)
{
    int total;
    int channels;

    if (!g_xbox_initialized || !samples || frames <= 0)
        return -1;

    channels = (__fb_sfx && __fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : FB_SFX_DEFAULT_CHANNELS;

    fb_sfxDriverDiagnostics("XAudio", samples, frames, channels);

    total = 0;
    while (total < frames)
    {
        int chunk;
        int written;

        chunk = frames - total;
        if (chunk > g_xbox_buffer_frames)
            chunk = g_xbox_buffer_frames;

        written = xbox_driver_write_chunk(samples + (total * FB_SFX_XBOX_CHANNELS),
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

static const FB_SFX_DRIVER fb_sfxDriverXboxXAudio =
{
    "xaudio",
    0,
    xbox_driver_init,
    xbox_driver_exit,
    xbox_driver_write,
    NULL,
    NULL,
    NULL,
    NULL
};

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverXboxXAudio,
    &__fb_sfxDriverNull,
    NULL
};


/* ------------------------------------------------------------------------- */
/* MIDI stubs                                                                */
/* ------------------------------------------------------------------------- */

int fb_sfxMidiDriverOpen(int device)
{
    (void)device;
    return -1;
}

void fb_sfxMidiDriverClose(void)
{
}

int fb_sfxMidiDriverSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2)
{
    (void)status;
    (void)data1;
    (void)data2;
    return -1;
}


/* end of sfx_driver_xaudio.c */
