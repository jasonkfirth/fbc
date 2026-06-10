/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_output_capture.c

    Purpose:

        Record the final sfxlib output stream into an in-memory buffer
        that can be saved as a WAV file by low-level callers.

    Responsibilities:

        - start and stop output-side recording
        - append driver-accepted mixer frames
        - reserve capture storage for known-duration exports
        - write recorded output as 16-bit PCM WAV

    This file intentionally does NOT contain:

        - BASIC command syntax
        - input-device capture
        - mixer voice generation
        - platform driver code

    Architectural overview:

        mixer/raw queue -> driver write -> output capture -> WAV file
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Output capture state                                                      */
/* ------------------------------------------------------------------------- */

typedef struct FB_SFX_OUTPUT_CAPTURE
{
    int active;
    int samplerate;
    int channels;
    int frames;
    int capacity_frames;
    float *samples;
} FB_SFX_OUTPUT_CAPTURE;

static FB_SFX_OUTPUT_CAPTURE g_output_capture;


/* ------------------------------------------------------------------------- */
/* Internal helpers                                                          */
/* ------------------------------------------------------------------------- */

static int fb_sfxOutputCaptureFrameSize(int channels, size_t *frame_size)
{
    if (!frame_size)
        return -1;

    if (channels <= 0)
        return -1;

    if ((size_t)channels > ((size_t)-1) / sizeof(float))
        return -1;

    *frame_size = (size_t)channels * sizeof(float);
    return 0;
}

static int fb_sfxOutputCaptureEnsureCapacityLocked(int wanted_frames)
{
    int new_capacity;
    size_t frame_size;
    size_t byte_count;
    float *new_samples;

    if (wanted_frames < 0)
        return -1;

    if (wanted_frames <= g_output_capture.capacity_frames)
        return 0;

    if (fb_sfxOutputCaptureFrameSize(g_output_capture.channels, &frame_size) != 0)
        return -1;

    if ((size_t)wanted_frames > ((size_t)-1) / frame_size)
        return -1;

    new_capacity = g_output_capture.capacity_frames;

    if (new_capacity <= 0)
    {
        new_capacity = g_output_capture.samplerate;

        if (new_capacity <= 0)
            new_capacity = FB_SFX_DEFAULT_RATE;
    }

    while (new_capacity < wanted_frames)
    {
        if (new_capacity > INT_MAX / 2)
        {
            new_capacity = wanted_frames;
            break;
        }

        new_capacity *= 2;
    }

    byte_count = (size_t)new_capacity * frame_size;
    new_samples = (float*)realloc(g_output_capture.samples, byte_count);

    if (!new_samples)
        return -1;

    g_output_capture.samples = new_samples;
    g_output_capture.capacity_frames = new_capacity;

    return 0;
}

static int fb_sfxOutputCaptureWriteBytes(FILE *file,
                                         const void *data,
                                         size_t bytes)
{
    if (bytes == 0)
        return 0;

    if (!file || !data)
        return -1;

    return (fwrite(data, 1, bytes, file) == bytes) ? 0 : -1;
}

static int fb_sfxOutputCaptureWriteU16(FILE *file, unsigned int value)
{
    unsigned char bytes[2];

    bytes[0] = (unsigned char)(value & 0xffu);
    bytes[1] = (unsigned char)((value >> 8) & 0xffu);

    return fb_sfxOutputCaptureWriteBytes(file, bytes, sizeof(bytes));
}

static int fb_sfxOutputCaptureWriteU32(FILE *file, unsigned int value)
{
    unsigned char bytes[4];

    bytes[0] = (unsigned char)(value & 0xffu);
    bytes[1] = (unsigned char)((value >> 8) & 0xffu);
    bytes[2] = (unsigned char)((value >> 16) & 0xffu);
    bytes[3] = (unsigned char)((value >> 24) & 0xffu);

    return fb_sfxOutputCaptureWriteBytes(file, bytes, sizeof(bytes));
}

static int fb_sfxOutputCaptureWriteHeader(FILE *file,
                                          int samplerate,
                                          int channels,
                                          int frames)
{
    unsigned int block_align;
    unsigned int byte_rate;
    unsigned int data_size;
    unsigned int file_size;
    unsigned long long data_bytes;

    if (!file || samplerate <= 0 || channels <= 0 || frames < 0)
        return -1;

    block_align = (unsigned int)channels * 2u;
    byte_rate = (unsigned int)samplerate * block_align;
    data_bytes = (unsigned long long)(unsigned int)frames *
                 (unsigned long long)block_align;

    if (data_bytes > (unsigned long long)(UINT_MAX - 36u))
        return -1;

    data_size = (unsigned int)data_bytes;
    file_size = 36u + data_size;

    if (fb_sfxOutputCaptureWriteBytes(file, "RIFF", 4) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteU32(file, file_size) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteBytes(file, "WAVE", 4) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteBytes(file, "fmt ", 4) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteU32(file, 16u) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteU16(file, 1u) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteU16(file, (unsigned int)channels) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteU32(file, (unsigned int)samplerate) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteU32(file, byte_rate) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteU16(file, block_align) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteU16(file, 16u) != 0)
        return -1;

    if (fb_sfxOutputCaptureWriteBytes(file, "data", 4) != 0)
        return -1;

    return fb_sfxOutputCaptureWriteU32(file, data_size);
}


/* ------------------------------------------------------------------------- */
/* Output capture lifecycle                                                  */
/* ------------------------------------------------------------------------- */

int fb_sfxOutputCaptureStart(void)
{
    int samplerate;
    int channels;

    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();

    samplerate = (__fb_sfx && __fb_sfx->samplerate > 0)
        ? __fb_sfx->samplerate
        : FB_SFX_DEFAULT_RATE;

    channels = (__fb_sfx && __fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : FB_SFX_DEFAULT_CHANNELS;

    if (g_output_capture.samples &&
        g_output_capture.channels > 0 &&
        g_output_capture.channels != channels)
    {
        free(g_output_capture.samples);
        g_output_capture.samples = NULL;
        g_output_capture.capacity_frames = 0;
    }

    g_output_capture.samplerate = samplerate;
    g_output_capture.channels = channels;
    g_output_capture.frames = 0;
    g_output_capture.active = 1;

    fb_sfxRuntimeUnlock();
    return 0;
}

int fb_sfxOutputCaptureReserve(int frames)
{
    int result;

    if (frames < 0)
        return -1;

    fb_sfxRuntimeLock();

    if (!g_output_capture.active &&
        g_output_capture.samplerate <= 0)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    result = fb_sfxOutputCaptureEnsureCapacityLocked(frames);

    fb_sfxRuntimeUnlock();
    return result;
}

void fb_sfxOutputCaptureStop(void)
{
    fb_sfxRuntimeLock();
    g_output_capture.active = 0;
    fb_sfxRuntimeUnlock();
}

void fb_sfxOutputCaptureShutdown(void)
{
    fb_sfxRuntimeLock();

    free(g_output_capture.samples);
    g_output_capture.samples = NULL;
    g_output_capture.active = 0;
    g_output_capture.samplerate = 0;
    g_output_capture.channels = 0;
    g_output_capture.frames = 0;
    g_output_capture.capacity_frames = 0;

    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Output capture append                                                     */
/* ------------------------------------------------------------------------- */

void fb_sfxOutputCaptureAppendLocked(const float *samples,
                                     int frames,
                                     int channels)
{
    size_t sample_count;
    size_t byte_count;

    if (!g_output_capture.active)
        return;

    if (!samples || frames <= 0 || channels <= 0)
        return;

    if (channels != g_output_capture.channels)
        return;

    if (frames > INT_MAX - g_output_capture.frames)
    {
        g_output_capture.active = 0;
        SFX_DEBUG("sfx_output_capture: frame count overflow");
        return;
    }

    if (fb_sfxOutputCaptureEnsureCapacityLocked(
            g_output_capture.frames + frames) != 0)
    {
        g_output_capture.active = 0;
        SFX_DEBUG("sfx_output_capture: failed to grow capture buffer");
        return;
    }

    sample_count = (size_t)frames * (size_t)channels;
    byte_count = sample_count * sizeof(float);

    memcpy(g_output_capture.samples +
           ((size_t)g_output_capture.frames * (size_t)channels),
           samples,
           byte_count);

    g_output_capture.frames += frames;
}


/* ------------------------------------------------------------------------- */
/* Output capture save                                                       */
/* ------------------------------------------------------------------------- */

int fb_sfxOutputCaptureSave(const char *filename)
{
    FILE *file;
    short *pcm;
    int frames_done;
    int result;
    int chunk_frames;

    if (!filename || !*filename)
        return -1;

    fb_sfxRuntimeLock();

    if (g_output_capture.active ||
        g_output_capture.samplerate <= 0 ||
        g_output_capture.channels <= 0 ||
        g_output_capture.frames < 0 ||
        (g_output_capture.frames > 0 && !g_output_capture.samples))
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    file = fopen(filename, "wb");
    if (!file)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    result = fb_sfxOutputCaptureWriteHeader(file,
                                            g_output_capture.samplerate,
                                            g_output_capture.channels,
                                            g_output_capture.frames);

    chunk_frames = 4096;
    pcm = NULL;

    if (result == 0 && g_output_capture.frames > 0)
    {
        pcm = (short*)malloc((size_t)chunk_frames *
                             (size_t)g_output_capture.channels *
                             sizeof(short));

        if (!pcm)
            result = -1;
    }

    frames_done = 0;

    while (result == 0 && frames_done < g_output_capture.frames)
    {
        int todo;
        int samples;
        size_t bytes;

        todo = g_output_capture.frames - frames_done;
        if (todo > chunk_frames)
            todo = chunk_frames;

        samples = todo * g_output_capture.channels;

        fb_sfxConvertFloatToS16(
            g_output_capture.samples +
            ((size_t)frames_done * (size_t)g_output_capture.channels),
            pcm,
            samples);

        bytes = (size_t)samples * sizeof(short);

        if (fb_sfxOutputCaptureWriteBytes(file, pcm, bytes) != 0)
            result = -1;

        frames_done += todo;
    }

    free(pcm);

    fb_sfxRuntimeUnlock();

    if (fclose(file) != 0)
        result = -1;

    return result;
}


/* end of sfx_output_capture.c */
