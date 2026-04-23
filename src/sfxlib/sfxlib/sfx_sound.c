/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_sound.c

    Purpose:

        Implement the BASIC SOUND command family.

        The SOUND command generates tones with a specific frequency
        and duration. This implementation maps the command onto the
        internal voice/oscillator system.

    Responsibilities:

        • create tone voices
        • configure oscillator frequency
        • apply duration control
        • attach voices to channels

    This file intentionally does NOT contain:

        • oscillator implementation
        • envelope processing
        • mixer logic
        • platform driver interaction

    Architectural overview:

        SOUND command
              │
              ▼
        voice allocation
              │
              ▼
        oscillator + envelope
              │
              ▼
        mixer → buffer → driver
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Default parameters                                                        */
/* ------------------------------------------------------------------------- */

#define FB_SFX_SOUND_VOLUME 0.8f


/* ------------------------------------------------------------------------- */
/* SOUND frequency, duration                                                 */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSound()

    Generate a tone with the specified frequency and duration.

    The tone is assigned to channel 0 by default.
*/

void fb_sfxSound(int frequency, float duration)
{
    fb_sfxSoundChannel(0, frequency, duration, FB_SFX_SOUND_VOLUME);
}


/* ------------------------------------------------------------------------- */
/* SOUND channel, frequency, duration, volume                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSoundChannel()

    Extended SOUND implementation allowing explicit channel and
    volume control.
*/

void fb_sfxSoundChannel(
    int channel,
    int frequency,
    float duration,
    float volume)
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

    if (volume < 0.0f)
        volume = 0.0f;

    if (volume > 1.0f)
        volume = 1.0f;

    voice = fb_sfxVoiceAlloc();

    if (!voice)
        return;

    voice->type = FB_SFX_VOICE_SOUND;
    voice->channel = channel;
    voice->volume = volume;

    fb_sfxInstrumentApply(voice, channel, FB_SFX_WAVE_TRIANGLE, 0);
    fb_sfxVoiceSetFrequency(voice, frequency);

    /* store duration in samples */

    if (__fb_sfx->samplerate > 0)
    {
        voice->length =
            (int)(duration * (float)__fb_sfx->samplerate);
    }

    voice->position = 0;

    SFX_DEBUG(
        "sfx_sound: ch=%d freq=%d dur=%f vol=%f",
        channel,
        frequency,
        duration,
        volume
    );
}


/* ------------------------------------------------------------------------- */
/* SOUND stop                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSoundStop()

    Stop all currently playing tones.
*/

void fb_sfxSoundStop(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxVoiceStopType(FB_SFX_VOICE_SOUND);

    SFX_DEBUG("sfx_sound: stop all tones");
}


/* ------------------------------------------------------------------------- */
/* SOUND stop channel                                                        */
/* ------------------------------------------------------------------------- */

void fb_sfxSoundStopChannel(int channel)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    fb_sfxVoiceStopTypeChannel(FB_SFX_VOICE_SOUND, channel);

    SFX_DEBUG("sfx_sound: stop channel %d", channel);
}


/* end of sfx_sound.c */
