/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_noise.c

    Purpose:

        Implement the BASIC NOISE command.

        The NOISE command generates non-periodic audio useful for
        percussion and sound effects. Internally this command
        configures a voice to use the noise oscillator.

    Responsibilities:

        • allocate noise voices
        • configure noise waveform
        • apply duration and volume
        • assign the voice to a channel

    This file intentionally does NOT contain:

        • oscillator implementation
        • envelope processing
        • mixer logic
        • platform driver interaction

    Architectural overview:

        NOISE command
             │
             ▼
        voice allocation
             │
             ▼
        noise oscillator
             │
             ▼
        envelope → mixer → buffer → driver
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Default parameters                                                        */
/* ------------------------------------------------------------------------- */

#define FB_SFX_NOISE_VOLUME 0.8f


/* ------------------------------------------------------------------------- */
/* NOISE channel, duration, volume                                           */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxNoise()

    Generate a noise burst on the specified channel.

    Parameters:

        channel   channel index
        duration  noise duration in seconds
        volume    output amplitude (0.0 – 1.0)
*/

void fb_sfxNoise(int channel, float duration, float volume)
{
    FB_SFXVOICE *voice;

    if (!fb_sfxEnsureInitialized())
        return;

    if (duration <= 0.0f)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        channel = 0;

    if (volume < 0.0f)
        volume = 0.0f;

    if (volume > 1.0f)
        volume = 1.0f;

    voice = fb_sfxVoiceAlloc();

    if (!voice)
        return;

    voice->type = FB_SFX_VOICE_NOISE;
    voice->channel = channel;
    voice->volume = volume;

    /* configure oscillator */

    fb_sfxVoiceSetWaveform(voice, FB_SFX_WAVE_NOISE);

    /*
        Noise oscillators do not use frequency in the
        traditional sense. The field is left unused.
    */

    voice->frequency = 0;

    /* compute duration in samples */

    if (__fb_sfx->samplerate > 0)
    {
        voice->length =
            (int)(duration * (float)__fb_sfx->samplerate);
    }

    voice->position = 0;

    /* assign default envelope */

    fb_sfxVoiceSetEnvelope(voice, 0);

    SFX_DEBUG(
        "sfx_noise: channel=%d duration=%f volume=%f",
        channel,
        duration,
        volume
    );
}


/* ------------------------------------------------------------------------- */
/* NOISE stop channel                                                        */
/* ------------------------------------------------------------------------- */

void fb_sfxNoiseStop(int channel)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    fb_sfxVoiceStopTypeChannel(FB_SFX_VOICE_NOISE, channel);

    SFX_DEBUG("sfx_noise: stop channel %d", channel);
}


/* ------------------------------------------------------------------------- */
/* NOISE stop all                                                            */
/* ------------------------------------------------------------------------- */

void fb_sfxNoiseStopAll(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxVoiceStopType(FB_SFX_VOICE_NOISE);

    SFX_DEBUG("sfx_noise: stop all noise");
}


/* end of sfx_noise.c */
