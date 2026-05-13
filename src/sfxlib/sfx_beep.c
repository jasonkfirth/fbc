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

#include <math.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Default BEEP parameters                                                   */
/* ------------------------------------------------------------------------- */

#define FB_SFX_BEEP_FREQ     880
#define FB_SFX_BEEP_DURATION 0.15f
#define FB_SFX_BEEP_VOLUME   0.8f
#define FB_SFX_BEEP_MIDDLE_C 261.6255653005986
#define FB_SFX_BEEP_MIN_FREQ 1
#define FB_SFX_BEEP_MAX_FREQ 20000


static int fb_sfxBeepDurationFrames(float duration)
{
    int frames;

    if (!__fb_sfx || __fb_sfx->samplerate <= 0 || duration <= 0.0f)
        return 0;

    frames = (int)(duration * (float)__fb_sfx->samplerate + 0.5f);
    if (frames <= 0)
        frames = 1;

    return frames;
}

static void fb_sfxBeepRunForeground(float duration)
{
    int frames;

    frames = fb_sfxBeepDurationFrames(duration);
    if (frames <= 0)
        return;

    fb_sfxRunForeground(frames);
}


/* ------------------------------------------------------------------------- */
/* Historical pitch conversion                                               */
/* ------------------------------------------------------------------------- */

static int fb_sfxBeepPitchToFrequency(float pitch)
{
    double frequency;

    /*
        Sinclair ZX Spectrum BASIC defines BEEP pitch as semitones above
        middle C.  Fractional pitch values are valid, so pow() is used here
        instead of a small integer note table.
    */

    frequency = FB_SFX_BEEP_MIDDLE_C * pow(2.0, (double)pitch / 12.0);

    if (frequency < (double)FB_SFX_BEEP_MIN_FREQ)
        return FB_SFX_BEEP_MIN_FREQ;

    if (frequency > (double)FB_SFX_BEEP_MAX_FREQ)
        return FB_SFX_BEEP_MAX_FREQ;

    return (int)(frequency + 0.5);
}


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
    if (!fb_sfxEnsureInitialized())
        return;

    if (frequency <= 0)
        frequency = FB_SFX_BEEP_FREQ;

    if (duration <= 0.0f)
        duration = FB_SFX_BEEP_DURATION;

    fb_sfxSoundQueue(0,
                     frequency,
                     duration,
                     FB_SFX_BEEP_VOLUME,
                     FB_SFX_WAVE_SQUARE,
                     0);

    SFX_DEBUG(
        "sfx_beep: freq=%d duration=%f",
        frequency,
        duration
    );
}


/* ------------------------------------------------------------------------- */
/* Spectrum-style BEEP                                                       */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxBeepPitch()

    Implements the historical two-argument BEEP form used by Sinclair
    ZX Spectrum BASIC:

        BEEP duration, pitch

    duration is measured in seconds, and pitch is measured in semitones
    above middle C.  The command is foreground because Spectrum BASIC
    programs depend on consecutive BEEP statements playing in sequence.
*/

void fb_sfxBeepPitch(float duration, float pitch)
{
    int frequency;

    if (duration <= 0.0f)
        return;

    frequency = fb_sfxBeepPitchToFrequency(pitch);
    fb_sfxBeepEx(frequency, duration);
    fb_sfxBeepRunForeground(duration);

    SFX_DEBUG(
        "sfx_beep: pitch=%f freq=%d duration=%f",
        pitch,
        frequency,
        duration
    );
}


/* end of sfx_beep.c */
