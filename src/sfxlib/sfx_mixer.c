/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_mixer.c

    Purpose:

        Implement the core software mixer used by the FreeBASIC sound
        subsystem.

        The mixer combines all active voices into a single output stream
        that is written into the runtime mix buffer.

    Responsibilities:

        • voice accumulation
        • channel volume and panning
        • envelope processing
        • sample clamping
        • generating the final audio frame stream

    This file intentionally does NOT contain:

        • platform driver code
        • BASIC command parsing
        • device enumeration logic
        • capture subsystem logic

    Architectural overview:

        voices → mixer → runtime mix buffer → audio driver

    Design note:

        The mixer operates entirely in floating point to simplify
        mixing logic and prevent overflow during accumulation.
*/

#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_mixer.h"

static float fb_sfxMixerVoiceSample(FB_SFXVOICE *voice)
{
    if (!voice)
        return 0.0f;

    if (voice->data)
    {
        float sample = 0.0f;

        if (voice->length <= 0)
        {
            voice->active = 0;
            return 0.0f;
        }

        if (voice->position >= voice->length)
        {
            if (voice->loop)
                voice->position = 0;
            else
            {
                voice->active = 0;
                return 0.0f;
            }
        }

        sample = voice->data[voice->position];
        voice->position++;

        return sample;
    }

    if (voice->length > 0 && voice->position >= voice->length)
    {
        if (voice->hard_stop)
        {
            voice->active = 0;
            return 0.0f;
        }

        if (voice->env_state != FB_SFX_ENV_RELEASE)
            fb_sfxEnvelopeRelease(voice);
    }

    if (voice->env_state != FB_SFX_ENV_RELEASE)
        voice->position++;

    {
        float sample = fb_sfxOscillatorSample(voice);

        if (voice->hard_stop &&
            voice->length > 0 &&
            voice->position >= voice->length)
        {
            voice->active = 0;
        }

        return sample;
    }
}


/* ------------------------------------------------------------------------- */
/* Mixer initialization                                                      */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerInit(void)
{
    int i;

    if (!__fb_sfx)
        return;

    SFX_DEBUG("sfx_mixer: initializing mixer");

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        __fb_sfx->voices[i].active = 0;
        __fb_sfx->voices[i].position = 0;
    }
}


/* ------------------------------------------------------------------------- */
/* Mixer shutdown                                                            */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerShutdown(void)
{
    if (!__fb_sfx)
        return;

    SFX_DEBUG("sfx_mixer: shutting down mixer");
}


/* ------------------------------------------------------------------------- */
/* Mixer buffer clear                                                        */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerClear(float *buffer, int frames)
{
    int samples;

    if (!buffer)
        return;

    samples = frames * __fb_sfx->output_channels;

    memset(buffer, 0, samples * sizeof(float));
}


/* ------------------------------------------------------------------------- */
/* Voice allocation                                                          */
/* ------------------------------------------------------------------------- */

FB_SFXVOICE *fb_sfxMixerAllocVoice(void)
{
    int i;

    if (!__fb_sfx)
        return NULL;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (!__fb_sfx->voices[i].active)
        {
            FB_SFXVOICE *v = &__fb_sfx->voices[i];

            memset(v, 0, sizeof(FB_SFXVOICE));

            v->active = 1;
            v->volume = 1.0f;
            v->pan = 0.0f;

            return v;
        }
    }

    return NULL;
}


/* ------------------------------------------------------------------------- */
/* Voice release                                                             */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerFreeVoice(FB_SFXVOICE *voice)
{
    if (!voice)
        return;

    voice->active = 0;
}


/* ------------------------------------------------------------------------- */
/* Stop voices                                                               */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerStopChannel(int channel)
{
    int i;

    if (!__fb_sfx)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (__fb_sfx->voices[i].active &&
            __fb_sfx->voices[i].channel == channel)
        {
            __fb_sfx->voices[i].active = 0;
        }
    }
}


void fb_sfxMixerStopAll(void)
{
    int i;

    if (!__fb_sfx)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
        __fb_sfx->voices[i].active = 0;
}


/* ------------------------------------------------------------------------- */
/* Channel control                                                           */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerSetChannelVolume(int channel, float volume)
{
    if (!__fb_sfx)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    __fb_sfx->channels[channel].volume = volume;
}


void fb_sfxMixerSetChannelPan(int channel, float pan)
{
    if (!__fb_sfx)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    __fb_sfx->channels[channel].pan = pan;
}


/* ------------------------------------------------------------------------- */
/* Envelope processing                                                       */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerProcessEnvelope(FB_SFXVOICE *voice)
{
    if (!voice || !__fb_sfx || __fb_sfx->samplerate <= 0)
        return;

    (void)fb_sfxEnvelopeProcess(voice, 1.0f / (float)__fb_sfx->samplerate);
}


/* ------------------------------------------------------------------------- */
/* Oscillator helper                                                         */
/* ------------------------------------------------------------------------- */

float fb_sfxMixerOscillator(FB_SFXVOICE *voice)
{
    if (!voice)
        return 0.0f;

    return fb_sfxMixerVoiceSample(voice);
}


/* ------------------------------------------------------------------------- */
/* Sample clamp                                                              */
/* ------------------------------------------------------------------------- */

float fb_sfxMixerClamp(float v)
{
    if (v > 1.0f)
        return 1.0f;

    if (v < -1.0f)
        return -1.0f;

    return v;
}


/* ------------------------------------------------------------------------- */
/* Mixer process                                                             */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerProcess(int frames)
{
    int frame;
    int voice;
    int ch;

    float left;
    float right;
    float master_volume;
    float balance;

    float *buffer;

    if (!__fb_sfx)
        return;

    buffer = __fb_sfx->mixbuffer;

    if (!buffer)
        return;

    fb_sfxMixerClear(buffer, frames);
    master_volume = __fb_sfx->master_volume;
    balance = __fb_sfx->balance;

    for (frame = 0; frame < frames; frame++)
    {
        left = 0.0f;
        right = 0.0f;

        for (voice = 0; voice < FB_SFX_MAX_VOICES; voice++)
        {
            FB_SFXVOICE *v = &__fb_sfx->voices[voice];

            if (!v->active)
                continue;

            if (v->paused)
                continue;

            if (v->start_delay > 0)
            {
                v->start_delay--;
                continue;
            }

            float sample;
            float env_level;

            sample = fb_sfxMixerOscillator(v);

            if (!v->active)
                continue;

            fb_sfxMixerProcessEnvelope(v);
            env_level = v->env_level;

            ch = v->channel;

            float volume = v->volume;

            if (ch >= 0 && ch < FB_SFX_MAX_CHANNELS)
            {
                if (__fb_sfx->channels[ch].mute)
                    continue;

                volume *= __fb_sfx->channels[ch].volume;
            }

            sample *= env_level * volume * master_volume;

            if (v->type == FB_SFX_VOICE_MUSIC && __fb_sfx->music_playing >= 0)
                __fb_sfx->music_pos = v->position;

            float pan = v->pan;

            if (ch >= 0 && ch < FB_SFX_MAX_CHANNELS)
                pan += __fb_sfx->channels[ch].pan;

            pan += balance;

            if (pan < -1.0f)
                pan = -1.0f;

            if (pan > 1.0f)
                pan = 1.0f;

            left  += sample * (1.0f - pan) * 0.5f;
            right += sample * (1.0f + pan) * 0.5f;
        }

        left  = fb_sfxMixerClamp(left);
        right = fb_sfxMixerClamp(right);

        buffer[frame * 2]     = left;
        buffer[frame * 2 + 1] = right;
    }
}


/* end of sfx_mixer.c */
