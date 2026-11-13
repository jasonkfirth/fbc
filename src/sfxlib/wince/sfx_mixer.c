/*
    FreeBASIC Sound Library support for Windows CE
    -----------------------------------------------

    File: sfx_mixer.c

    Purpose:

        Implement the Windows CE copy of the core software mixer.

        The mixer combines all active voices into a single output stream
        that is written into the runtime mix buffer.

        The software MIDI fallback contributes its FM voices here so they
        follow the same ring-buffer and output-driver path.

    Responsibilities:

        - voice accumulation
        - channel volume and panning
        - envelope processing
        - sample clamping
        - generating the final audio frame stream

    This file intentionally does NOT contain:

        - platform driver code
        - BASIC command parsing
        - device enumeration logic
        - capture subsystem logic
        - device-screen diagnostics or environment-variable controls

    Architectural overview:

        voices -> mixer -> runtime mix buffer -> audio driver

    Design note:

        The mixer operates entirely in floating point to simplify
        mixing logic and prevent overflow during accumulation.
*/

#include <math.h>
#include <limits.h>
#include <string.h>

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_mixer.h"

static float fb_sfxMixerVoiceSample(FB_SFXVOICE *voice)
{
    if (!voice)
        return 0.0f;

    if (voice->data)
    {
        float sample = 0.0f;
        float next_sample = 0.0f;
        float fraction;
        int index;
        int next_index;

        if (voice->length <= 0)
        {
            voice->active = 0;
            return 0.0f;
        }

        if (voice->sample_step <= 0.0f)
            voice->sample_step = 1.0f;

        if (voice->sample_pos < 0.0f)
            voice->sample_pos = 0.0f;

        while (voice->sample_pos >= (float)voice->length)
        {
            if (voice->loop)
                voice->sample_pos -= (float)voice->length;
            else
            {
                voice->active = 0;
                return 0.0f;
            }
        }

        index = (int)voice->sample_pos;
        next_index = index + 1;
        fraction = voice->sample_pos - (float)index;

        if (next_index >= voice->length)
        {
            if (voice->loop)
                next_index = 0;
            else
                next_index = index;
        }

        sample = voice->data[index];
        next_sample = voice->data[next_index];

        sample += (next_sample - sample) * fraction;

        voice->sample_pos += voice->sample_step;

        while (voice->sample_pos >= (float)voice->length && voice->loop)
            voice->sample_pos -= (float)voice->length;

        if (voice->sample_pos >= (float)voice->length)
            voice->position = voice->length;
        else
            voice->position = (int)voice->sample_pos;

        voice->pos = voice->position;

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

void fb_sfxMixerDiagnostics(const float *buffer, int frames)
{
    (void)buffer;
    (void)frames;
}

int fb_sfxMixerSimdEnabled(void)
{
    /* The supported Windows CE ARM and MIPS baselines have no SIMD backend. */
    return 0;
}


/* ------------------------------------------------------------------------- */
/* Mixer initialization                                                      */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerInit(void)
{
    int i;

    if (!fb_sfx_context)
        return;

    SFX_DEBUG("sfx_mixer: initializing mixer");

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        fb_sfx_context->voices[i].active = 0;
        fb_sfx_context->voices[i].position = 0;
    }
}


/* ------------------------------------------------------------------------- */
/* Mixer shutdown                                                            */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerShutdown(void)
{
    if (!fb_sfx_context)
        return;

    SFX_DEBUG("sfx_mixer: shutting down mixer");
}


/* ------------------------------------------------------------------------- */
/* Mixer buffer clear                                                        */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerClear(float *buffer, int frames)
{
    int samples;

    if (!buffer || !fb_sfx_context || frames <= 0 ||
        fb_sfx_context->output_channels <= 0)
        return;

    if (frames > INT_MAX / fb_sfx_context->output_channels)
        return;

    samples = frames * fb_sfx_context->output_channels;

    memset(buffer, 0, (size_t)samples * sizeof(float));
}


/* ------------------------------------------------------------------------- */
/* Voice allocation                                                          */
/* ------------------------------------------------------------------------- */

FB_SFXVOICE *fb_sfxMixerAllocVoice(void)
{
    int i;

    if (!fb_sfx_context)
        return NULL;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (!fb_sfx_context->voices[i].active)
        {
            FB_SFXVOICE *v = &fb_sfx_context->voices[i];

            memset(v, 0, sizeof(FB_SFXVOICE));

            v->active = 1;
            v->volume = 1.0f;
            v->pan = 0.0f;
            v->sample_step = 1.0f;

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

    if (!fb_sfx_context)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (fb_sfx_context->voices[i].active &&
            fb_sfx_context->voices[i].channel == channel)
        {
            fb_sfx_context->voices[i].active = 0;
        }
    }
}


void fb_sfxMixerStopAll(void)
{
    int i;

    if (!fb_sfx_context)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
        fb_sfx_context->voices[i].active = 0;
}


/* ------------------------------------------------------------------------- */
/* Channel control                                                           */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerSetChannelVolume(int channel, float volume)
{
    if (!fb_sfx_context)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    fb_sfx_context->channels[channel].volume = volume;
}


void fb_sfxMixerSetChannelPan(int channel, float pan)
{
    if (!fb_sfx_context)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    fb_sfx_context->channels[channel].pan = pan;
}


/* ------------------------------------------------------------------------- */
/* Envelope processing                                                       */
/* ------------------------------------------------------------------------- */

void fb_sfxMixerProcessEnvelope(FB_SFXVOICE *voice)
{
    if (!voice || !fb_sfx_context || fb_sfx_context->samplerate <= 0)
        return;

    (void)fb_sfxEnvelopeProcess(voice, 1.0f / (float)fb_sfx_context->samplerate);
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

    if (!fb_sfx_context)
        return;

    buffer = fb_sfx_context->mixbuffer;

    if (!buffer)
        return;

    fb_sfxMixerClear(buffer, frames);
    master_volume = fb_sfx_context->master_volume;
    balance = fb_sfx_context->balance;

    for (frame = 0; frame < frames; frame++)
    {
        left = 0.0f;
        right = 0.0f;

        for (voice = 0; voice < FB_SFX_MAX_VOICES; voice++)
        {
            FB_SFXVOICE *v = &fb_sfx_context->voices[voice];

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
                if (fb_sfx_context->channels[ch].mute)
                    continue;

                volume *= fb_sfx_context->channels[ch].volume;
            }

            sample *= env_level * volume * master_volume;

            if (v->type == FB_SFX_VOICE_MUSIC && fb_sfx_context->music_playing >= 0)
                fb_sfx_context->music_pos = v->position;

            float pan = v->pan;

            if (ch >= 0 && ch < FB_SFX_MAX_CHANNELS)
                pan += fb_sfx_context->channels[ch].pan;

            pan += balance;

            if (pan < -1.0f)
                pan = -1.0f;

            if (pan > 1.0f)
                pan = 1.0f;

            left  += sample * (1.0f - pan) * 0.5f;
            right += sample * (1.0f + pan) * 0.5f;
        }

        /*
            The software MIDI fallback owns a separate, bounded FM voice
            pool. Add it to the completed sfxlib voice mix so its waveform
            follows the same effects, capture, and output-ring path.
        */
        fb_sfxMidiSoftwareMixFrame(&left, &right);

        /*
            Global effects operate on the completed stereo frame. Applying
            them here preserves each voice's own envelope and panning while
            keeping the delay line independent of the platform driver.
        */
        fb_sfxEchoProcess(&left, &right);

        left  = fb_sfxMixerClamp(left);
        right = fb_sfxMixerClamp(right);

        buffer[frame * 2]     = left;
        buffer[frame * 2 + 1] = right;
    }

}


/* end of sfx_mixer.c */
