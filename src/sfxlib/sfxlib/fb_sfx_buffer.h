/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_buffer.h

    Purpose:

        Define internal buffer management interfaces used by the
        FreeBASIC sound subsystem.

        The buffer layer provides the runtime-owned memory structures
        that connect the mixer, audio drivers, streaming subsystem,
        and audio capture system.

    Responsibilities:

        • audio ring buffer management
        • mixer output buffering
        • capture buffering
        • safe producer/consumer audio pipelines

    This file intentionally does NOT contain:

        • audio synthesis logic
        • mixer implementation
        • platform driver implementations
        • BASIC command handlers

    Architectural overview:

        mixer → runtime mix buffer → driver write()

        capture driver → runtime capture buffer → BASIC program

        streaming audio → ring buffer → mixer
*/

#ifndef __FB_SFX_BUFFER_H__
#define __FB_SFX_BUFFER_H__

#include "fb_sfx.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Ring buffer structure                                                     */
/* ------------------------------------------------------------------------- */

/*
    FB_SFX_RINGBUFFER

    Lock-safe ring buffer used by multiple subsystems:

        • audio streaming
        • driver output queues
        • capture input buffering

    The ring buffer is designed for a simple producer/consumer model
    where one component writes data and another reads it.
*/

typedef struct FB_SFX_RINGBUFFER
{
    int frames;
    int channels;
    int size;

    int read_pos;
    int write_pos;

    int count;

    float *data;
    float *buffer;

} FB_SFX_RINGBUFFER;

typedef FB_SFX_RINGBUFFER FB_SFXRINGBUFFER;



/* ------------------------------------------------------------------------- */
/* Ring buffer lifecycle                                                     */
/* ------------------------------------------------------------------------- */

/*
    Initialize a ring buffer.

    Parameters:

        rb      ring buffer structure
        size    buffer capacity (frames)
*/

int fb_sfxRingBufferInit(
    FB_SFX_RINGBUFFER *rb,
    int frames,
    int channels
);


/*
    Shutdown a ring buffer.

    Releases allocated memory.
*/

void fb_sfxRingBufferShutdown(
    FB_SFX_RINGBUFFER *rb
);



/* ------------------------------------------------------------------------- */
/* Ring buffer operations                                                    */
/* ------------------------------------------------------------------------- */

/*
    Write samples to a ring buffer.

    Returns number of frames successfully written.
*/

int fb_sfxRingBufferWrite(
    FB_SFX_RINGBUFFER *rb,
    const float *samples,
    int frames
);


/*
    Read samples from a ring buffer.

    Returns number of frames successfully read.
*/

int fb_sfxRingBufferRead(
    FB_SFX_RINGBUFFER *rb,
    float *samples,
    int frames
);

void fb_sfxRingBufferClear(FB_SFX_RINGBUFFER *rb);
int fb_sfxRingBufferAvailableRead(FB_SFX_RINGBUFFER *rb);
int fb_sfxRingBufferAvailableWrite(FB_SFX_RINGBUFFER *rb);
float fb_sfxRingBufferPeek(FB_SFX_RINGBUFFER *rb, int frame, int channel);



/* ------------------------------------------------------------------------- */
/* Ring buffer status                                                        */
/* ------------------------------------------------------------------------- */

/*
    Return number of frames currently stored in the buffer.
*/

int fb_sfxRingBufferAvailable(
    const FB_SFX_RINGBUFFER *rb
);


/*
    Return number of free frames available for writing.
*/

int fb_sfxRingBufferFree(
    const FB_SFX_RINGBUFFER *rb
);



/* ------------------------------------------------------------------------- */
/* Mixer output buffer                                                       */
/* ------------------------------------------------------------------------- */

/*
    Mixer output buffer

    The mixer writes audio frames into the runtime-owned buffer.
    Platform drivers read from this buffer when delivering audio
    to the operating system.

    This ensures deterministic behavior and allows automated tests
    to inspect generated audio without requiring audio hardware.
*/

void fb_sfxMixBufferInit(void);
void fb_sfxMixBufferShutdown(void);

int fb_sfxMixBufferWrite(
    const float *samples,
    int frames
);

int fb_sfxMixBufferRead(
    float *samples,
    int frames
);



/* ------------------------------------------------------------------------- */
/* Capture buffer                                                            */
/* ------------------------------------------------------------------------- */

/*
    Capture buffer

    The capture subsystem stores audio samples obtained from
    microphone or line input devices.

    BASIC programs can read samples from this buffer using
    capture commands.
*/

void fb_sfxCaptureBufferInit(void);
void fb_sfxCaptureBufferShutdown(void);

int fb_sfxCaptureBufferWrite(
    const short *samples,
    int frames
);

int fb_sfxCaptureBufferRead(
    short *samples,
    int frames
);



/* ------------------------------------------------------------------------- */
/* Stream buffer                                                             */
/* ------------------------------------------------------------------------- */

/*
    Stream buffers are used by audio file playback and streaming
    sources.

    Streaming data is decoded into these buffers and consumed
    by the mixer.
*/

void fb_sfxStreamBufferInit(void);
void fb_sfxStreamBufferShutdown(void);



#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_buffer.h */
