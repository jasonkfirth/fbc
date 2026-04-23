/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_waveform.c

    Purpose:

        Implement waveform generation used by the sound synthesis engine.

        Waveforms are the fundamental signal sources used by voices.

    Responsibilities:

        • oscillator generation
        • waveform lookup
        • phase advancement
        • noise generation

    This file intentionally does NOT contain:

        • envelope processing
        • mixer logic
        • driver interaction
        • command parsing

    Architectural overview:

        voice
          │
          ▼
        waveform generator
          │
          ▼
        envelope
          │
          ▼
        mixer
*/

#include <math.h>
#include <stdlib.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Constants                                                                 */
/* ------------------------------------------------------------------------- */

#define FB_SFX_PI 3.14159265358979323846


/* ------------------------------------------------------------------------- */
/* Phase advance helper                                                      */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxWaveAdvance()

    Advance oscillator phase based on voice frequency.

    The phase range is normalized:

        0.0 → start of waveform
        1.0 → end of waveform
*/

static void fb_sfxWaveAdvance(FB_SFXVOICE *v)
{
    float step;

    if (!v)
        return;

    if (!__fb_sfx)
        return;

    if (__fb_sfx->samplerate <= 0)
        return;

    step = (float)v->frequency / (float)__fb_sfx->samplerate;

    v->phase += step;

    if (v->phase >= 1.0f)
        v->phase -= 1.0f;
}


/* ------------------------------------------------------------------------- */
/* Sine oscillator                                                           */
/* ------------------------------------------------------------------------- */

static float fb_sfxWaveSine(FB_SFXVOICE *v)
{
    float sample;

    sample = sinf(v->phase * 2.0f * FB_SFX_PI);

    fb_sfxWaveAdvance(v);

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Square oscillator                                                         */
/* ------------------------------------------------------------------------- */

static float fb_sfxWaveSquare(FB_SFXVOICE *v)
{
    float sample;

    if (v->phase < 0.5f)
        sample = 1.0f;
    else
        sample = -1.0f;

    fb_sfxWaveAdvance(v);

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Triangle oscillator                                                       */
/* ------------------------------------------------------------------------- */

static float fb_sfxWaveTriangle(FB_SFXVOICE *v)
{
    float sample;

    if (v->phase < 0.25f)
        sample = v->phase * 4.0f;
    else if (v->phase < 0.75f)
        sample = 2.0f - v->phase * 4.0f;
    else
        sample = v->phase * 4.0f - 4.0f;

    fb_sfxWaveAdvance(v);

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Sawtooth oscillator                                                       */
/* ------------------------------------------------------------------------- */

static float fb_sfxWaveSaw(FB_SFXVOICE *v)
{
    float sample;

    sample = (v->phase * 2.0f) - 1.0f;

    fb_sfxWaveAdvance(v);

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Noise generator                                                           */
/* ------------------------------------------------------------------------- */

static float fb_sfxWaveNoise(FB_SFXVOICE *v)
{
    float sample;

    sample = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;

    (void)v;

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Waveform dispatcher                                                       */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxWaveformSample()

    Generate one sample from a voice's configured waveform.
*/

float fb_sfxWaveformSample(FB_SFXVOICE *v)
{
    if (!v)
        return 0.0f;

    switch (v->waveform)
    {
        case FB_SFX_WAVE_SINE:
            return fb_sfxWaveSine(v);

        case FB_SFX_WAVE_SQUARE:
            return fb_sfxWaveSquare(v);

        case FB_SFX_WAVE_TRIANGLE:
            return fb_sfxWaveTriangle(v);

        case FB_SFX_WAVE_SAW:
            return fb_sfxWaveSaw(v);

        case FB_SFX_WAVE_NOISE:
            return fb_sfxWaveNoise(v);
    }

    return 0.0f;
}


/* ------------------------------------------------------------------------- */
/* Waveform configuration                                                    */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxVoiceSetWaveform()

    Assign a waveform generator to a voice.
*/

void fb_sfxVoiceSetWaveform(FB_SFXVOICE *v, int waveform)
{
    if (!v)
        return;

    v->waveform = waveform;

    SFX_DEBUG("sfx_waveform: voice waveform set to %d", waveform);
}


/* end of sfx_waveform.c */
