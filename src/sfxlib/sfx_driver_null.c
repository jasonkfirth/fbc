/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_null.c

    Purpose:

        Implement a null audio backend.

        The null driver accepts audio buffers but discards them.
        It allows the sound system to initialize successfully
        even when no real audio backend is available.

    Responsibilities:

        • provide a safe fallback audio driver
        • allow mixer and command testing without hardware
        • maintain compatibility with the driver interface

    This file intentionally does NOT contain:

        • real audio device interaction
        • platform-specific code
        • mixer logic
*/

#include "fb_sfx.h"
#include "fb_sfx_driver.h"
#include "fb_sfx_driver_diag.h"
#include "fb_sfx_internal.h"

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#endif


/* ------------------------------------------------------------------------- */
/* Driver state                                                              */
/* ------------------------------------------------------------------------- */

static int null_initialized = 0;
static int null_channels = FB_SFX_DEFAULT_CHANNELS;
#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
static int null_rate = FB_SFX_DEFAULT_RATE;
static int null_buffer_frames = FB_SFX_DEFAULT_BUFFER;
static unsigned int null_qemu_write_count = 0;
static uint32_t null_qemu_last_checksum = 0;
static uint32_t null_qemu_last_pcm_checksum = 0;
#endif


/* ------------------------------------------------------------------------- */
/* Driver init                                                               */
/* ------------------------------------------------------------------------- */

static int null_driver_init(int rate, int channels, int buffer_size, int flags)
{
    (void)flags;

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
    null_rate = (rate > 0) ? rate : FB_SFX_DEFAULT_RATE;
    null_buffer_frames = (buffer_size > 0) ? buffer_size : FB_SFX_DEFAULT_BUFFER;
#else
    (void)rate;
    (void)buffer_size;
#endif
    null_channels = (channels > 0) ? channels : FB_SFX_DEFAULT_CHANNELS;
    null_initialized = 1;

    SFX_DEBUG("sfx_driver_null: initialized");

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Driver shutdown                                                           */
/* ------------------------------------------------------------------------- */

static void null_driver_shutdown(void)
{
    null_initialized = 0;
    null_channels = FB_SFX_DEFAULT_CHANNELS;
#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
    null_rate = FB_SFX_DEFAULT_RATE;
    null_buffer_frames = FB_SFX_DEFAULT_BUFFER;
    null_qemu_write_count = 0;
    null_qemu_last_checksum = 0;
    null_qemu_last_pcm_checksum = 0;
#endif

    SFX_DEBUG("sfx_driver_null: shutdown");
}


/* ------------------------------------------------------------------------- */
/* Driver write                                                              */
/* ------------------------------------------------------------------------- */

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
static uint32_t null_qemu_checksum_samples(const float *buffer, int frames,
    int channels)
{
    const unsigned char *data;
    size_t bytes;
    size_t i;
    uint32_t hash;

    if ((buffer == NULL) || (frames <= 0) || (channels <= 0))
        return 0;

    bytes = (size_t)frames * (size_t)channels * sizeof(float);
    data = (const unsigned char *)buffer;

    /*
        FNV-1a gives the QEMU smoke harness a stable signature without
        copying audio into a file.  This is not a hardware audio proof; it
        proves that the mixer produced samples and the backend consumed them.
    */
    hash = 2166136261u;

    for (i = 0; i < bytes; i++) {
        hash ^= (uint32_t)data[i];
        hash *= 16777619u;
    }

    return hash;
}

static void null_qemu_trace_write(const float *buffer, int frames)
{
    int samples;
    int i;
    int nonzero;
    float min_sample;
    float max_sample;
    uint32_t checksum;

    checksum = null_qemu_checksum_samples(buffer, frames, null_channels);

    if ((null_qemu_write_count > 0) &&
        (checksum == null_qemu_last_checksum))
        return;

    null_qemu_write_count++;
    null_qemu_last_checksum = checksum;

    samples = 0;
    nonzero = 0;
    min_sample = 0.0f;
    max_sample = 0.0f;

    if ((buffer != NULL) && (frames > 0) && (null_channels > 0)) {
        samples = frames * null_channels;

        if (samples > 0) {
            min_sample = buffer[0];
            max_sample = buffer[0];
        }

        /*
            A checksum proves repeatability.  These small statistics prove the
            mixer delivered an actual waveform to the driver boundary.
        */
        for (i = 0; i < samples; i++) {
            if (buffer[i] != 0.0f)
                nonzero++;

            if (buffer[i] < min_sample)
                min_sample = buffer[i];

            if (buffer[i] > max_sample)
                max_sample = buffer[i];
        }
    }

    printf("FB_NUTTX_QEMU_SFX_WRITE write=%u frames=%d channels=%d samples=%d nonzero=%d min=%ld max=%ld checksum=%08lx\n",
        null_qemu_write_count,
        frames,
        null_channels,
        samples,
        nonzero,
        (long)(min_sample * 1000000.0f),
        (long)(max_sample * 1000000.0f),
        (unsigned long)checksum);
    fflush(stdout);
}

static uint32_t null_qemu_pcm_hash_word(uint32_t hash, short sample)
{
    unsigned short word;

    word = (unsigned short)sample;
    hash ^= (uint32_t)(word & 0xffu);
    hash *= 16777619u;
    hash ^= (uint32_t)((word >> 8) & 0xffu);
    hash *= 16777619u;

    return hash;
}

static void null_qemu_trace_pcm(const float *buffer, int frames)
{
    int samples;
    int i;
    int active;
    int peak;
    long long sum_abs;
    uint32_t checksum;

    if ((buffer == NULL) || (frames <= 0) || (null_channels <= 0))
        return;

    if (frames > (INT_MAX / null_channels))
        return;

    /*
        The RP2350 HDMI/DVI audio path is not implemented yet, but a real
        backend is expected to consume ordinary signed 16-bit PCM after the
        mixer.  This mock trace proves that conversion boundary without
        pretending QEMU is driving the board's electrical audio transport.
    */
    samples = frames * null_channels;
    active = 0;
    peak = 0;
    sum_abs = 0;
    checksum = 2166136261u;

    for (i = 0; i < samples; i++) {
        short pcm;
        int magnitude;

        pcm = fb_sfxFloatToS16(buffer[i]);
        magnitude = (pcm < 0) ? -(int)pcm : (int)pcm;

        if (magnitude != 0)
            active++;

        if (magnitude > peak)
            peak = magnitude;

        sum_abs += magnitude;
        checksum = null_qemu_pcm_hash_word(checksum, pcm);
    }

    if ((null_qemu_write_count > 0) &&
        (checksum == null_qemu_last_pcm_checksum))
        return;

    null_qemu_last_pcm_checksum = checksum;

    printf("FB_NUTTX_QEMU_SFX_PCM rate=%d channels=%d bits=16 frames=%d samples=%d active=%d peak=%d mean_abs=%ld buffer_frames=%d checksum=%08lx\n",
        null_rate,
        null_channels,
        frames,
        samples,
        active,
        peak,
        (long)(sum_abs / samples),
        null_buffer_frames,
        (unsigned long)checksum);
    fflush(stdout);
}
#endif

/*
    Accept audio samples but discard them.

    This allows the mixer and buffer pipeline to operate
    normally during testing.
*/

static int null_driver_write(const float *buffer, int frames)
{
    if (!null_initialized)
        return -1;

    fb_sfxDriverDiagnostics("null", buffer, frames, null_channels);

#ifdef FB_NUTTX_QEMU_MOCK_DEVICES
    null_qemu_trace_write(buffer, frames);
    null_qemu_trace_pcm(buffer, frames);
#endif

    /* intentionally discard audio */

    return frames;
}


/* ------------------------------------------------------------------------- */
/* Driver status                                                             */
/* ------------------------------------------------------------------------- */

const FB_SFX_DRIVER __fb_sfxDriverNull =
{
    "null",
    0,
    null_driver_init,
    null_driver_shutdown,
    null_driver_write,
    NULL,
    NULL,
    NULL,
    NULL
};

/*
    Some platform driver lists use the non-prefixed internal spelling.  Keep
    this separate object until those target adapters converge on one symbol.
*/
const FB_SFX_DRIVER fb_sfxDriverNull =
{
    "null",
    0,
    null_driver_init,
    null_driver_shutdown,
    null_driver_write,
    NULL,
    NULL,
    NULL,
    NULL
};


/* end of sfx_driver_null.c */
