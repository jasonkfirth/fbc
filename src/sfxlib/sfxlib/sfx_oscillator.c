/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_oscillator.c

    Purpose:

        Implement oscillator generation used by active voices.

        Oscillators convert waveform definitions and frequency
        values into sample streams used by the mixer.

    Responsibilities:

        • oscillator phase management
        • waveform sample generation
        • frequency stepping
        • oscillator reset

    This file intentionally does NOT contain:

        • envelope processing
        • mixer logic
        • driver interaction
        • command parsing

    Architectural overview:

        waveform definition
                │
                ▼
        oscillator engine
                │
                ▼
        envelope processor
                │
                ▼
        mixer
*/

#include <math.h>
#include <stdlib.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#define FB_SFX_PI 3.14159265358979323846


/* ------------------------------------------------------------------------- */
/* Oscillator reset                                                          */
/* ------------------------------------------------------------------------- */

void fb_sfxOscillatorReset(FB_SFXVOICE *voice)
{
    if (!voice)
        return;

    voice->phase = 0.0f;
}


/* ------------------------------------------------------------------------- */
/* Phase step calculation                                                    */
/* ------------------------------------------------------------------------- */

static float fb_sfxOscillatorStep(FB_SFXVOICE *voice)
{
    if (!voice)
        return 0.0f;

    if (!__fb_sfx)
        return 0.0f;

    if (__fb_sfx->samplerate <= 0)
        return 0.0f;

    return (float)voice->frequency / (float)__fb_sfx->samplerate;
}


/* ------------------------------------------------------------------------- */
/* Phase advance                                                             */
/* ------------------------------------------------------------------------- */

static void fb_sfxOscillatorAdvance(FB_SFXVOICE *voice)
{
    float step;

    if (!voice)
        return;

    step = fb_sfxOscillatorStep(voice);

    voice->phase += step;

    while (voice->phase >= 1.0f)
        voice->phase -= 1.0f;
}


/* ------------------------------------------------------------------------- */
/* Sine oscillator                                                           */
/* ------------------------------------------------------------------------- */

static float fb_sfxOscillatorSine(FB_SFXVOICE *voice)
{
    float sample;

    sample = sinf(voice->phase * 2.0f * FB_SFX_PI);

    fb_sfxOscillatorAdvance(voice);

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Square oscillator                                                         */
/* ------------------------------------------------------------------------- */

static float fb_sfxOscillatorSquare(FB_SFXVOICE *voice)
{
    float sample;

    if (voice->phase < 0.5f)
        sample = 1.0f;
    else
        sample = -1.0f;

    fb_sfxOscillatorAdvance(voice);

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Triangle oscillator                                                       */
/* ------------------------------------------------------------------------- */

static float fb_sfxOscillatorTriangle(FB_SFXVOICE *voice)
{
    float p = voice->phase;
    float sample;

    if (p < 0.25f)
        sample = p * 4.0f;
    else if (p < 0.75f)
        sample = 2.0f - p * 4.0f;
    else
        sample = p * 4.0f - 4.0f;

    fb_sfxOscillatorAdvance(voice);

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Saw oscillator                                                            */
/* ------------------------------------------------------------------------- */

static float fb_sfxOscillatorSaw(FB_SFXVOICE *voice)
{
    float sample;

    sample = (voice->phase * 2.0f) - 1.0f;

    fb_sfxOscillatorAdvance(voice);

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Noise oscillator                                                          */
/* ------------------------------------------------------------------------- */

static float fb_sfxOscillatorNoise(FB_SFXVOICE *voice)
{
    float sample;

    (void)voice;

    sample = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;

    return sample;
}


/* ------------------------------------------------------------------------- */
/* Oscillator dispatcher                                                     */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxOscillatorSample()

    Generate a sample from a voice oscillator.
*/

float fb_sfxOscillatorSample(FB_SFXVOICE *voice)
{
    if (!voice)
        return 0.0f;

    switch (voice->waveform)
    {
        case FB_SFX_WAVE_SINE:
            return fb_sfxOscillatorSine(voice);

        case FB_SFX_WAVE_SQUARE:
            return fb_sfxOscillatorSquare(voice);

        case FB_SFX_WAVE_TRIANGLE:
            return fb_sfxOscillatorTriangle(voice);

        case FB_SFX_WAVE_SAW:
            return fb_sfxOscillatorSaw(voice);

        case FB_SFX_WAVE_NOISE:
            return fb_sfxOscillatorNoise(voice);
    }

    return 0.0f;
}


/* ------------------------------------------------------------------------- */
/* Frequency control                                                         */
/* ------------------------------------------------------------------------- */

void fb_sfxVoiceSetFrequency(FB_SFXVOICE *voice, int freq)
{
    if (!voice)
        return;

    if (freq < 0)
        freq = 0;

    voice->frequency = freq;

    SFX_DEBUG("sfx_oscillator: frequency set to %d", freq);
}


/* end of sfx_oscillator.c */
