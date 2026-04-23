/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_beep.c

    Purpose:

        Implement the BASIC BEEP command.

        Historically, BEEP produced a short audible tone through
        the PC speaker. In the modern sfxlib architecture this
        command generates a short synthesized tone using the
        internal oscillator and envelope systems.

    Responsibilities:

        • provide a simple audible feedback tone
        • allocate a temporary voice
        • configure waveform and envelope

    This file intentionally does NOT contain:

        • oscillator implementation
        • mixer logic
        • driver interaction
        • command parsing

    Architectural overview:

        BEEP command
             │
             ▼
        voice allocation
             │
             ▼
        waveform + envelope
             │
             ▼
        mixer → buffer → driver
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Default BEEP parameters                                                   */
/* ------------------------------------------------------------------------- */

#define FB_SFX_BEEP_FREQ     880
#define FB_SFX_BEEP_DURATION 0.15f
#define FB_SFX_BEEP_VOLUME   0.8f


/* ------------------------------------------------------------------------- */
/* BEEP implementation                                                       */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxBeep()

    Generate a short audible tone.

    The implementation intentionally uses a square wave oscillator
    because this most closely resembles the original PC speaker
    sound used by early BASIC systems.
*/

void fb_sfxBeep(void)
{
    fb_sfxBeepEx(FB_SFX_BEEP_FREQ, FB_SFX_BEEP_DURATION);

    SFX_DEBUG("sfx_beep: generated beep");
}


/* ------------------------------------------------------------------------- */
/* Parameterized BEEP                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxBeepEx()

    Extended BEEP implementation allowing custom frequency
    and duration.

    This is used internally by SOUND-style commands.
*/

void fb_sfxBeepEx(int frequency, float duration)
{
    FB_SFXVOICE *voice;

    if (!fb_sfxEnsureInitialized())
        return;

    if (frequency <= 0)
        frequency = FB_SFX_BEEP_FREQ;

    if (duration <= 0.0f)
        duration = FB_SFX_BEEP_DURATION;

    voice = fb_sfxVoiceAlloc();

    if (!voice)
        return;

    voice->channel = 0;

    fb_sfxVoiceSetWaveform(voice, FB_SFX_WAVE_SQUARE);
    fb_sfxVoiceSetFrequency(voice, frequency);

    voice->volume = FB_SFX_BEEP_VOLUME;
    voice->length = 0;

    if (__fb_sfx->samplerate > 0)
        voice->length = (int)(duration * (float)__fb_sfx->samplerate);

    voice->position = 0;

    fb_sfxVoiceSetEnvelope(voice, 0);

    SFX_DEBUG(
        "sfx_beep: freq=%d duration=%f",
        frequency,
        duration
    );
}


/* end of sfx_beep.c */
