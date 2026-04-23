/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_mixer.h

    Purpose:

        Define the internal interface for the software mixer used by
        the FreeBASIC sound subsystem.

        The mixer is responsible for combining all active voices into
        a single output stream that is delivered to the platform audio
        driver.

    Responsibilities:

        • voice mixing
        • channel volume and panning
        • envelope processing
        • oscillator generation
        • producing the final PCM output buffer

    This file intentionally does NOT contain:

        • platform audio driver implementations
        • BASIC command handlers
        • runtime initialization logic

    Architectural overview:

        BASIC command layer
                │
                ▼
        voice / instrument generation
                │
                ▼
        software mixer
                │
                ▼
        runtime mix buffer
                │
                ▼
        platform audio driver
*/

#ifndef __FB_SFX_MIXER_H__
#define __FB_SFX_MIXER_H__

#include "fb_sfx.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Mixer configuration                                                       */
/* ------------------------------------------------------------------------- */

/*
    Maximum frames processed in a single mixer iteration.

    This limit prevents large driver buffer requests from causing
    excessive CPU spikes in the mixer.
*/

#define FB_SFX_MIXER_MAX_FRAMES 4096



/* ------------------------------------------------------------------------- */
/* Mixer lifecycle                                                           */
/* ------------------------------------------------------------------------- */

/*
    Initialize the mixer subsystem.

    Called during runtime initialization after the global audio
    context has been created.
*/

void fb_sfxMixerInit(void);


/*
    Shutdown the mixer subsystem.

    Releases any internal resources owned by the mixer.
*/

void fb_sfxMixerShutdown(void);



/* ------------------------------------------------------------------------- */
/* Mixing operations                                                         */
/* ------------------------------------------------------------------------- */

/*
    Mix audio frames.

    The mixer processes active voices and writes the resulting
    audio samples into the runtime mix buffer.

    Parameters:

        frames  number of audio frames to generate

    A frame contains one sample per channel.
*/

void fb_sfxMixerProcess(int frames);



/*
    Clear the mixer buffer.

    This helper resets the output buffer before mixing begins.
*/

void fb_sfxMixerClear(float *buffer, int frames);



/* ------------------------------------------------------------------------- */
/* Voice management                                                          */
/* ------------------------------------------------------------------------- */

/*
    Allocate a voice slot.

    Returns a pointer to a voice structure or NULL if no free
    voice slots are available.
*/

FB_SFXVOICE *fb_sfxMixerAllocVoice(void);


/*
    Release a voice slot.

    The voice becomes inactive and will no longer contribute
    samples to the mixer.
*/

void fb_sfxMixerFreeVoice(FB_SFXVOICE *voice);



/*
    Stop all voices associated with a channel.
*/

void fb_sfxMixerStopChannel(int channel);



/*
    Stop all active voices.
*/

void fb_sfxMixerStopAll(void);



/* ------------------------------------------------------------------------- */
/* Channel control                                                           */
/* ------------------------------------------------------------------------- */

/*
    Set channel volume.

    Volume is expressed as a normalized value where:

        0.0  = silent
        1.0  = full volume
*/

void fb_sfxMixerSetChannelVolume(int channel, float volume);



/*
    Set channel pan position.

    Pan range:

        -1.0  = full left
         0.0  = center
         1.0  = full right
*/

void fb_sfxMixerSetChannelPan(int channel, float pan);



/* ------------------------------------------------------------------------- */
/* Envelope processing                                                       */
/* ------------------------------------------------------------------------- */

/*
    Apply envelope processing to a voice.

    This function advances the ADSR envelope state and modifies
    the voice amplitude accordingly.
*/

void fb_sfxMixerProcessEnvelope(FB_SFXVOICE *voice);



/* ------------------------------------------------------------------------- */
/* Oscillator helpers                                                        */
/* ------------------------------------------------------------------------- */

/*
    Generate oscillator samples for a voice.

    The oscillator type determines how the waveform is produced.
*/

float fb_sfxMixerOscillator(FB_SFXVOICE *voice);



/* ------------------------------------------------------------------------- */
/* Utility helpers                                                           */
/* ------------------------------------------------------------------------- */

/*
    Clamp audio sample to valid output range.

    The mixer keeps sample values within [-1.0, 1.0] to avoid
    distortion or overflow when converting to integer formats.
*/

float fb_sfxMixerClamp(float v);



#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_mixer.h */
