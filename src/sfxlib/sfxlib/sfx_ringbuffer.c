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

    samples = frames * channels;
    size = samples * sizeof(float);

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

    size = rb->frames * rb->channels * sizeof(float);

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
    int written = 0;
    int ch;

    if (!rb || !src)
        return 0;

    while (written < frames && rb->count < rb->frames)
    {
        int index = rb->write_pos * rb->channels;

        for (ch = 0; ch < rb->channels; ch++)
        {
            rb->data[index + ch] =
                src[(written * rb->channels) + ch];
        }

        rb->write_pos++;

        if (rb->write_pos >= rb->frames)
            rb->write_pos = 0;

        rb->count++;
        written++;
    }

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
    int read = 0;
    int ch;

    if (!rb || !dst)
        return 0;

    while (read < frames && rb->count > 0)
    {
        int index = rb->read_pos * rb->channels;

        for (ch = 0; ch < rb->channels; ch++)
        {
            dst[(read * rb->channels) + ch] =
                rb->data[index + ch];
        }

        rb->read_pos++;

        if (rb->read_pos >= rb->frames)
            rb->read_pos = 0;

        rb->count--;
        read++;
    }

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
