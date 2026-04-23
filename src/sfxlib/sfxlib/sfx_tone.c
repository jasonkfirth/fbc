/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_tone.c

    Purpose:

        Implement the BASIC TONE command family.

        TONE generates a frequency on a specified channel for a
        defined duration. Unlike SOUND, which historically focused
        on single-tone playback, TONE is explicitly channel-oriented.

    Responsibilities:

        • channel-based tone generation
        • voice allocation
        • oscillator configuration
        • duration management

    This file intentionally does NOT contain:

        • oscillator implementation
        • envelope logic
        • mixer algorithms
        • platform driver code

    Architectural overview:

        TONE command
              │
              ▼
        voice configuration
              │
              ▼
        oscillator → envelope → mixer → buffer → driver
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Default tone parameters                                                   */
/* ------------------------------------------------------------------------- */

#define FB_SFX_TONE_VOLUME 0.8f


/* ------------------------------------------------------------------------- */
/* TONE channel, frequency, duration                                         */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxTone()

    Generate a tone on a specified channel.

    Parameters:

        channel   channel index
        frequency oscillator frequency in Hz
        duration  tone duration in seconds
*/

void fb_sfxTone(int channel, int frequency, float duration)
{
    FB_SFXVOICE *voice;

    if (!fb_sfxEnsureInitialized())
        return;

    if (frequency <= 0)
        return;

    if (duration <= 0.0f)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        channel = 0;

    voice = fb_sfxVoiceAlloc();

    if (!voice)
        return;

    voice->type = FB_SFX_VOICE_TONE;
    voice->channel = channel;
    voice->volume = FB_SFX_TONE_VOLUME;

    fb_sfxInstrumentApply(voice, channel, FB_SFX_WAVE_TRIANGLE, 0);
    fb_sfxVoiceSetFrequency(voice, frequency);

    if (__fb_sfx->samplerate > 0)
    {
        voice->length =
            (int)(duration * (float)__fb_sfx->samplerate);
    }

    voice->position = 0;

    SFX_DEBUG(
        "sfx_tone: channel=%d freq=%d dur=%f",
        channel,
        frequency,
        duration
    );
}


/* ------------------------------------------------------------------------- */
/* TONE stop channel                                                         */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxToneStop()

    Stop tone generation on a specific channel.
*/

void fb_sfxToneStop(int channel)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    fb_sfxVoiceStopTypeChannel(FB_SFX_VOICE_TONE, channel);

    SFX_DEBUG("sfx_tone: stop channel %d", channel);
}


/* ------------------------------------------------------------------------- */
/* TONE stop all                                                             */
/* ------------------------------------------------------------------------- */

void fb_sfxToneStopAll(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxVoiceStopType(FB_SFX_VOICE_TONE);

    SFX_DEBUG("sfx_tone: stop all tones");
}


/* end of sfx_tone.c */
