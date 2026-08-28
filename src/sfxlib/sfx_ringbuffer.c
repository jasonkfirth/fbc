/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_ringbuffer.c

    Purpose:

        Implement a lock-safe ring buffer used for transferring
        audio frames between the mixer subsystem and the platform
        audio driver.

    Responsibilities:

        • ring buffer allocation
        • read/write pointer management
        • overflow protection
        • frame availability tracking

    This file intentionally does NOT contain:

        • audio synthesis
        • mixer logic
        • driver playback code
        • command parsing

    Architectural overview:

        mixer → ring buffer → driver → OS audio
*/

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Ring buffer initialization                                                */
/* ------------------------------------------------------------------------- */

int fb_sfxRingBufferInit(FB_SFXRINGBUFFER *rb, int frames, int channels)
{
    int samples;
    size_t size;

    if (!rb)
        return -1;

    if (frames <= 0 || channels <= 0)
        return -1;

    if (frames > INT_MAX / channels)
        return -1;

    samples = frames * channels;

    if ((size_t)samples > ((size_t)-1) / sizeof(float))
        return -1;

    size = (size_t)samples * sizeof(float);

    rb->data = (float*)malloc(size);

    if (!rb->data)
        return -1;

    memset(rb->data, 0, size);

    rb->frames = frames;
    rb->channels = channels;

    rb->read_pos = 0;
    rb->write_pos = 0;
    rb->count = 0;

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Ring buffer shutdown                                                      */
/* ------------------------------------------------------------------------- */

void fb_sfxRingBufferShutdown(FB_SFXRINGBUFFER *rb)
{
    if (!rb)
        return;

    if (rb->data)
    {
        free(rb->data);
        rb->data = NULL;
    }

    rb->frames = 0;
    rb->channels = 0;
    rb->read_pos = 0;
    rb->write_pos = 0;
    rb->count = 0;
}


/* ------------------------------------------------------------------------- */
/* Ring buffer reset                                                         */
/* ------------------------------------------------------------------------- */

void fb_sfxRingBufferClear(FB_SFXRINGBUFFER *rb)
{
    size_t size;

    if (!rb)
        return;

    if (!rb->data)
        return;

    if (rb->frames <= 0 || rb->channels <= 0 ||
        rb->frames > INT_MAX / rb->channels)
        return;

    size = (size_t)(rb->frames * rb->channels) * sizeof(float);

    memset(rb->data, 0, size);

    rb->read_pos = 0;
    rb->write_pos = 0;
    rb->count = 0;
}


/* ------------------------------------------------------------------------- */
/* Frames available                                                          */
/* ------------------------------------------------------------------------- */

int fb_sfxRingBufferAvailableRead(FB_SFXRINGBUFFER *rb)
{
    if (!rb)
        return 0;

    return rb->count;
}


int fb_sfxRingBufferAvailableWrite(FB_SFXRINGBUFFER *rb)
{
    if (!rb)
        return 0;

    return rb->frames - rb->count;
}


int fb_sfxRingBufferAvailable(const FB_SFX_RINGBUFFER *rb)
{
    if (!rb)
        return 0;

    return rb->count;
}


int fb_sfxRingBufferFree(const FB_SFX_RINGBUFFER *rb)
{
    if (!rb)
        return 0;

    return rb->frames - rb->count;
}


/* ------------------------------------------------------------------------- */
/* Write frames                                                              */
/* ------------------------------------------------------------------------- */

int fb_sfxRingBufferWrite(
    FB_SFXRINGBUFFER *rb,
    const float *src,
    int frames)
{
    int first_frames;
    int writable;
    int written;
    size_t first_samples;
    size_t remaining_samples;

    if (!rb || !src || !rb->data || frames <= 0 ||
        rb->frames <= 0 || rb->channels <= 0 ||
        rb->frames > INT_MAX / rb->channels ||
        rb->read_pos < 0 || rb->read_pos >= rb->frames ||
        rb->write_pos < 0 || rb->write_pos >= rb->frames ||
        rb->count < 0 || rb->count > rb->frames)
    {
        return 0;
    }

    writable = rb->frames - rb->count;
    written = (frames < writable) ? frames : writable;
    if (written <= 0)
        return 0;

    first_frames = rb->frames - rb->write_pos;
    if (first_frames > written)
        first_frames = written;

    first_samples = (size_t)first_frames * (size_t)rb->channels;
    memcpy(rb->data + ((size_t)rb->write_pos * (size_t)rb->channels),
           src,
           first_samples * sizeof(float));

    remaining_samples = (size_t)(written - first_frames) *
                        (size_t)rb->channels;
    if (remaining_samples > 0)
    {
        memcpy(rb->data,
               src + first_samples,
               remaining_samples * sizeof(float));
    }

    rb->write_pos += written;
    if (rb->write_pos >= rb->frames)
        rb->write_pos -= rb->frames;

    rb->count += written;

    return written;
}


/* ------------------------------------------------------------------------- */
/* Read frames                                                               */
/* ------------------------------------------------------------------------- */

int fb_sfxRingBufferRead(
    FB_SFXRINGBUFFER *rb,
    float *dst,
    int frames)
{
    int first_frames;
    int read;
    size_t first_samples;
    size_t remaining_samples;

    if (!rb || !dst || !rb->data || frames <= 0 ||
        rb->frames <= 0 || rb->channels <= 0 ||
        rb->frames > INT_MAX / rb->channels ||
        rb->read_pos < 0 || rb->read_pos >= rb->frames ||
        rb->write_pos < 0 || rb->write_pos >= rb->frames ||
        rb->count < 0 || rb->count > rb->frames)
    {
        return 0;
    }

    read = (frames < rb->count) ? frames : rb->count;
    if (read <= 0)
        return 0;

    first_frames = rb->frames - rb->read_pos;
    if (first_frames > read)
        first_frames = read;

    first_samples = (size_t)first_frames * (size_t)rb->channels;
    memcpy(dst,
           rb->data + ((size_t)rb->read_pos * (size_t)rb->channels),
           first_samples * sizeof(float));

    remaining_samples = (size_t)(read - first_frames) *
                        (size_t)rb->channels;
    if (remaining_samples > 0)
    {
        memcpy(dst + first_samples,
               rb->data,
               remaining_samples * sizeof(float));
    }

    rb->read_pos += read;
    if (rb->read_pos >= rb->frames)
        rb->read_pos -= rb->frames;

    rb->count -= read;

    return read;
}


/* ------------------------------------------------------------------------- */
/* Peek frame                                                                */
/* ------------------------------------------------------------------------- */

float fb_sfxRingBufferPeek(
    FB_SFXRINGBUFFER *rb,
    int frame,
    int channel)
{
    int index;

    if (!rb)
        return 0.0f;

    if (!rb->data)
        return 0.0f;

    if (frame < 0 || frame >= rb->frames)
        return 0.0f;

    if (channel < 0 || channel >= rb->channels)
        return 0.0f;

    index = frame * rb->channels + channel;

    return rb->data[index];
}


/* end of sfx_ringbuffer.c */
