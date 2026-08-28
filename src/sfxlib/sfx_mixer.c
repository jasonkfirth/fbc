/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_mixer.c

    Purpose:

        Implement the core software mixer used by the FreeBASIC sound
        subsystem.

        The mixer combines all active voices into a single output stream
        that is written into the runtime mix buffer.

        The software MIDI fallback contributes its FM voices here so they
        follow the same ring-buffer and output-driver path.

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

#include <math.h>
#if !defined(_WIN32_WCE)
#include <errno.h>
#endif
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_mixer.h"
#include "sfx_simd.h"

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

static int fb_sfxMixerEnvEnabled(const char *name)
{
    const char *value = fb_sfxGetEnv(name);

    return (value && *value && *value != '0');
}

static int fb_sfxMixerDebugEnabled(void)
{
    static int initialized = 0;
    static int enabled = 0;

    if (!initialized)
    {
        initialized = 1;
        enabled = fb_sfxMixerEnvEnabled("SFXLIB_MIXER_DEBUG");
    }

    return enabled;
}

static FILE *fb_sfxMixerDumpFile(void)
{
    static int initialized = 0;
    static FILE *file = NULL;

    if (!initialized)
    {
        const char *path = fb_sfxGetEnv("SFXLIB_MIXER_DUMP");

        initialized = 1;

        if (path && *path)
            file = fb_sfxOpenFile(path, "w");
    }

    return file;
}

static int fb_sfxMixerDumpFrameLimit(void)
{
    static int initialized = 0;
    static int limit = 44100;

    if (!initialized)
    {
        const char *value = fb_sfxGetEnv("SFXLIB_MIXER_DUMP_FRAMES");

        initialized = 1;

        if (value && *value)
        {
            char *endptr;
            long parsed;

#if !defined(_WIN32_WCE)
            errno = 0;
#endif
            parsed = strtol(value, &endptr, 10);

            if (
#if !defined(_WIN32_WCE)
                (errno == 0) &&
#endif
                (endptr != value) && (*endptr == '\0') &&
                (parsed > 0) && (parsed <= INT_MAX))
            {
                limit = (int)parsed;
            }
        }
    }

    return limit;
}

void fb_sfxMixerDiagnostics(const float *buffer, int frames)
{
    static int block_count = 0;
    static int dumped_frames = 0;
    FILE *dump;
    int debug_enabled;
    int dump_limit;
    int channels;
    int samples;
    int active;
    int i;
    float peak;
    double sum_squares;

    if (!__fb_sfx || !buffer || frames <= 0)
        return;

    channels = (__fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : FB_SFX_DEFAULT_CHANNELS;
    samples = frames * channels;

    debug_enabled = fb_sfxMixerDebugEnabled() && block_count < 32;
    dump = fb_sfxMixerDumpFile();
    dump_limit = dump ? fb_sfxMixerDumpFrameLimit() : 0;

    /*
        Diagnostics are called for every mixed block.  Do not scan the audio
        buffer when neither diagnostic consumer is active.  In particular,
        peak and RMS calculation must not become a permanent tax on ordinary
        playback merely because the hooks are compiled into the runtime.
    */
    if (!debug_enabled && (!dump || dumped_frames >= dump_limit))
    {
        block_count++;
        return;
    }

    if (debug_enabled)
    {
        peak = 0.0f;
        sum_squares = 0.0;

        for (i = 0; i < samples; i++)
        {
            float value = buffer[i];
            float magnitude = (value < 0.0f) ? -value : value;

            if (magnitude > peak)
                peak = magnitude;

            sum_squares += (double)value * (double)value;
        }

        active = 0;
        for (i = 0; i < FB_SFX_MAX_VOICES; i++)
        {
            if (__fb_sfx->voices[i].active)
                active++;
        }

        fprintf(stderr,
                "SFX_MIXER: block=%d frames=%d active=%d peak=%0.6f rms=%0.6f\n",
                block_count,
                frames,
                active,
                peak,
                (samples > 0) ? sqrt(sum_squares / (double)samples) : 0.0);
    }

    block_count++;

    if (dump && dumped_frames < dump_limit)
    {
        int remaining = dump_limit - dumped_frames;
        int dump_frames = (frames < remaining) ? frames : remaining;

        for (i = 0; i < dump_frames; i++)
        {
            float mono;

            if (channels >= 2)
                mono = (buffer[i * channels] + buffer[i * channels + 1]) * 0.5f;
            else
                mono = buffer[i * channels];

            fprintf(dump, "%0.9f\n", mono);
        }

        dumped_frames += dump_frames;
        fflush(dump);
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

    if (!buffer || !__fb_sfx || frames <= 0 ||
        __fb_sfx->output_channels <= 0)
        return;

    if (frames > INT_MAX / __fb_sfx->output_channels)
        return;

    samples = frames * __fb_sfx->output_channels;

    memset(buffer, 0, (size_t)samples * sizeof(float));
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

static void fb_sfxMixerFinishBlock(float *buffer,
                                    int frames,
                                    int use_simd);

static void fb_sfxMixerProcessFrameMajor(int frames)
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

        buffer[frame * 2]     = left;
        buffer[frame * 2 + 1] = right;
    }

    /* Noise ordering is frame-major; completed-buffer work need not be. */
    fb_sfxMixerFinishBlock(buffer, frames, fb_sfxMixerSimdEnabled());
}


/* ------------------------------------------------------------------------- */
/* Block-oriented mixer                                                      */
/* ------------------------------------------------------------------------- */

typedef struct FB_SFX_MIXER_VOICE_GAINS
{
    float volume;
    float left;
    float right;
    int muted;
} FB_SFX_MIXER_VOICE_GAINS;

#define FB_SFX_MIXER_SAMPLE_BATCH 64

static int fb_sfxMixerHasSharedNoise(int frames)
{
    int voice;

    if (!__fb_sfx || frames <= 0)
        return 0;

    for (voice = 0; voice < FB_SFX_MAX_VOICES; voice++)
    {
        const FB_SFXVOICE *v = &__fb_sfx->voices[voice];

        if (v->active && !v->paused && !v->data &&
            v->waveform == FB_SFX_WAVE_NOISE)
        {
            return 1;
        }
    }

    return 0;
}

static void fb_sfxMixerVoiceGains(const FB_SFXVOICE *voice,
                                  float master_volume,
                                  float balance,
                                  FB_SFX_MIXER_VOICE_GAINS *gains)
{
    int channel;
    float pan;

    if (!voice || !gains)
        return;

    channel = voice->channel;
    gains->volume = voice->volume * master_volume;
    gains->muted = 0;
    pan = voice->pan + balance;

    if (channel >= 0 && channel < FB_SFX_MAX_CHANNELS)
    {
        gains->volume *= __fb_sfx->channels[channel].volume;
        gains->muted = __fb_sfx->channels[channel].mute ? 1 : 0;
        pan += __fb_sfx->channels[channel].pan;
    }

    if (pan < -1.0f)
        pan = -1.0f;
    if (pan > 1.0f)
        pan = 1.0f;

    gains->left = (1.0f - pan) * 0.5f;
    gains->right = (1.0f + pan) * 0.5f;
}

static void fb_sfxMixerRenderVoiceScalar(FB_SFXVOICE *voice,
                                         float *buffer,
                                         int frames,
                                         int first_frame,
                                         const FB_SFX_MIXER_VOICE_GAINS *gains)
{
    int frame;

    if (!voice || !buffer || !gains || frames <= 0 ||
        !voice->active || voice->paused)
    {
        return;
    }

    for (frame = first_frame; frame < frames && voice->active; frame++)
    {
        float env_level;
        float sample;

        sample = fb_sfxMixerVoiceSample(voice);

        if (!voice->active)
            continue;

        fb_sfxMixerProcessEnvelope(voice);
        env_level = voice->env_level;

        if (gains->muted)
            continue;

        sample *= env_level * gains->volume;

        if (voice->type == FB_SFX_VOICE_MUSIC &&
            __fb_sfx->music_playing >= 0)
        {
            __fb_sfx->music_pos = voice->position;
        }

        buffer[frame * 2] += sample * gains->left;
        buffer[frame * 2 + 1] += sample * gains->right;
    }
}

int fb_sfxMixerSimdEnabled(void)
{
#ifdef FB_SFX_HAS_SIMD
    static int initialized = 0;
    static int disabled = 0;
    unsigned int capabilities = fb_sfxSimdCapabilities();

    if (!initialized)
    {
        const char *setting = fb_sfxGetEnv("SFXLIB_MIXER_SIMD");

        /* A zero setting provides a scalar reference for diagnosis and tests. */
        disabled = setting && setting[0] == '0' && setting[1] == '\0';
        initialized = 1;
    }

    if (disabled)
        return 0;

    return (capabilities & (FB_SFX_SIMD_SSE2 | FB_SFX_SIMD_NEON)) != 0u;
#else
    return 0;
#endif
}

static void fb_sfxMixerAccumulateMono(const float *source,
                                      float *destination,
                                      int frames,
                                      float left_gain,
                                      float right_gain,
                                      int use_simd)
{
    int frame;

    if (!source || !destination || frames <= 0)
        return;

#ifdef FB_SFX_HAS_SIMD
    if (use_simd)
    {
        fb_sfxMixMonoToStereoSIMD(source,
                                  destination,
                                  frames,
                                  left_gain,
                                  right_gain);
        return;
    }
#else
    (void)use_simd;
#endif

    for (frame = 0; frame < frames; frame++)
    {
        float sample = source[frame];

        destination[frame * 2] += sample * left_gain;
        destination[frame * 2 + 1] += sample * right_gain;
    }
}

static int fb_sfxMixerConsumeStartDelay(FB_SFXVOICE *voice, int frames)
{
    int delayed_frames;

    if (!voice || frames <= 0 || voice->start_delay <= 0)
        return 0;

    delayed_frames = voice->start_delay;
    if (delayed_frames > frames)
        delayed_frames = frames;

    voice->start_delay -= delayed_frames;
    return delayed_frames;
}

static int fb_sfxMixerRenderContiguousSample(
    FB_SFXVOICE *voice,
    float *buffer,
    int frames,
    int first_frame,
    const FB_SFX_MIXER_VOICE_GAINS *gains,
    int use_simd)
{
    float envelope;
    float left_gain;
    float right_gain;
    int frame;

    if (!voice || !buffer || !gains || !voice->data ||
        voice->length <= 0 || voice->sample_step != 1.0f ||
        voice->env_state != FB_SFX_ENV_SUSTAIN)
    {
        return first_frame;
    }

    if (voice->envelope < 0 || voice->envelope >= FB_SFX_MAX_ENVELOPES)
        return first_frame;

    envelope = __fb_sfx->envelopes[voice->envelope].sustain;
    voice->env_level = envelope;
    left_gain = gains->muted
        ? 0.0f
        : envelope * gains->volume * gains->left;
    right_gain = gains->muted
        ? 0.0f
        : envelope * gains->volume * gains->right;
    frame = first_frame;

    while (frame < frames && voice->active)
    {
        int available;
        int position;
        int render_frames;

        if (voice->sample_pos < 0.0f)
            voice->sample_pos = 0.0f;

        while (voice->sample_pos >= (float)voice->length)
        {
            if (voice->loop)
                voice->sample_pos -= (float)voice->length;
            else
            {
                voice->active = 0;
                return frame;
            }
        }

        position = (int)voice->sample_pos;
        if (voice->sample_pos != (float)position)
            return frame;

        available = voice->length - position;
        render_frames = frames - frame;
        if (render_frames > available)
            render_frames = available;

        fb_sfxMixerAccumulateMono(voice->data + position,
                                  buffer + frame * 2,
                                  render_frames,
                                  left_gain,
                                  right_gain,
                                  use_simd);

        voice->sample_pos += (float)render_frames;
        frame += render_frames;

        if (voice->sample_pos >= (float)voice->length)
        {
            if (voice->loop)
                voice->sample_pos -= (float)voice->length;
            else if (frame < frames)
                voice->active = 0;
        }

        if (voice->sample_pos >= (float)voice->length)
            voice->position = voice->length;
        else
            voice->position = (int)voice->sample_pos;
        voice->pos = voice->position;

        if (!gains->muted &&
            voice->type == FB_SFX_VOICE_MUSIC &&
            __fb_sfx->music_playing >= 0)
        {
            __fb_sfx->music_pos = voice->position;
        }
    }

    return frame;
}

static int fb_sfxMixerRenderResampledVoice(
    FB_SFXVOICE *voice,
    float *buffer,
    int frames,
    int first_frame,
    const FB_SFX_MIXER_VOICE_GAINS *gains,
    int use_simd)
{
    float samples[FB_SFX_MIXER_SAMPLE_BATCH];
    float envelope;
    float left_gain;
    float right_gain;
    int frame;

    if (!voice || !buffer || !gains || !voice->data ||
        voice->length <= 0 || voice->sample_step <= 0.0f ||
        voice->sample_step == 1.0f ||
        voice->env_state != FB_SFX_ENV_SUSTAIN ||
        voice->envelope < 0 || voice->envelope >= FB_SFX_MAX_ENVELOPES)
    {
        return first_frame;
    }

    envelope = __fb_sfx->envelopes[voice->envelope].sustain;
    voice->env_level = envelope;
    left_gain = gains->muted
        ? 0.0f
        : envelope * gains->volume * gains->left;
    right_gain = gains->muted
        ? 0.0f
        : envelope * gains->volume * gains->right;
    frame = first_frame;

    while (frame < frames && voice->active)
    {
        int batch_frames = 0;
        int batch_limit = frames - frame;

        if (batch_limit > FB_SFX_MIXER_SAMPLE_BATCH)
            batch_limit = FB_SFX_MIXER_SAMPLE_BATCH;

        while (batch_frames < batch_limit && voice->active)
        {
            float fraction;
            float next_sample;
            float sample;
            int index;
            int next_index;

            if (voice->sample_pos < 0.0f)
                voice->sample_pos = 0.0f;

            while (voice->sample_pos >= (float)voice->length)
            {
                if (voice->loop)
                    voice->sample_pos -= (float)voice->length;
                else
                {
                    voice->active = 0;
                    break;
                }
            }

            if (!voice->active)
                break;

            index = (int)voice->sample_pos;
            next_index = index + 1;
            fraction = voice->sample_pos - (float)index;

            if (next_index >= voice->length)
                next_index = voice->loop ? 0 : index;

            sample = voice->data[index];
            next_sample = voice->data[next_index];
            samples[batch_frames] =
                sample + (next_sample - sample) * fraction;
            batch_frames++;

            voice->sample_pos += voice->sample_step;
            while (voice->sample_pos >= (float)voice->length && voice->loop)
                voice->sample_pos -= (float)voice->length;

            if (voice->sample_pos >= (float)voice->length)
                voice->position = voice->length;
            else
                voice->position = (int)voice->sample_pos;
            voice->pos = voice->position;
        }

        if (batch_frames <= 0)
            break;

        fb_sfxMixerAccumulateMono(samples,
                                  buffer + frame * 2,
                                  batch_frames,
                                  left_gain,
                                  right_gain,
                                  use_simd);
        frame += batch_frames;

        if (!gains->muted &&
            voice->type == FB_SFX_VOICE_MUSIC &&
            __fb_sfx->music_playing >= 0)
        {
            __fb_sfx->music_pos = voice->position;
        }
    }

    return frame;
}

static void fb_sfxMixerAccumulateWaveformScalar(int waveform,
                                                float *phase,
                                                float phase_step,
                                                float *buffer,
                                                int frames,
                                                float left_gain,
                                                float right_gain)
{
    float current_phase;
    int frame;

    if (!phase || !buffer || frames <= 0 || phase_step <= 0.0f)
        return;

    current_phase = *phase;

    for (frame = 0; frame < frames; frame++)
    {
        float sample = fb_sfxOscillatorWaveSample(waveform, current_phase);

        buffer[frame * 2] += sample * left_gain;
        buffer[frame * 2 + 1] += sample * right_gain;

        current_phase += phase_step;
        while (current_phase >= 1.0f)
            current_phase -= 1.0f;
    }

    *phase = current_phase;
}

static int fb_sfxMixerRenderSustainedWaveform(
    FB_SFXVOICE *voice,
    float *buffer,
    int frames,
    int first_frame,
    const FB_SFX_MIXER_VOICE_GAINS *gains,
    int use_simd)
{
    float envelope;
    float left_gain;
    float phase_step;
    float right_gain;
    int render_frames;

    if (!voice || !buffer || !gains || voice->data ||
        voice->waveform == FB_SFX_WAVE_NOISE ||
        voice->env_state != FB_SFX_ENV_SUSTAIN ||
        voice->envelope < 0 || voice->envelope >= FB_SFX_MAX_ENVELOPES)
    {
        return first_frame;
    }

    phase_step = fb_sfxOscillatorPhaseStep(voice);
    if (phase_step <= 0.0f)
        return first_frame;

    envelope = __fb_sfx->envelopes[voice->envelope].sustain;
    voice->env_level = envelope;
    left_gain = gains->muted
        ? 0.0f
        : envelope * gains->volume * gains->left;
    right_gain = gains->muted
        ? 0.0f
        : envelope * gains->volume * gains->right;
    render_frames = frames - first_frame;

    if (voice->length > 0)
    {
        int available = voice->length - voice->position;

        if (voice->hard_stop)
            available--;

        if (available < 0)
            available = 0;
        if (render_frames > available)
            render_frames = available;
    }

    if (render_frames > 0)
    {
#ifdef FB_SFX_HAS_SIMD
        if (use_simd &&
            (voice->waveform == FB_SFX_WAVE_SQUARE ||
             voice->waveform == FB_SFX_WAVE_TRIANGLE ||
             voice->waveform == FB_SFX_WAVE_SAW))
        {
            fb_sfxMixWaveformToStereoSIMD(voice->waveform,
                                          &voice->phase,
                                          phase_step,
                                          buffer + first_frame * 2,
                                          render_frames,
                                          left_gain,
                                          right_gain);
        }
        else
#else
        (void)use_simd;
#endif
        {
            fb_sfxMixerAccumulateWaveformScalar(voice->waveform,
                                                &voice->phase,
                                                phase_step,
                                                buffer + first_frame * 2,
                                                render_frames,
                                                left_gain,
                                                right_gain);
        }

        voice->position += render_frames;
        voice->pos = voice->position;
        first_frame += render_frames;

        if (!gains->muted &&
            voice->type == FB_SFX_VOICE_MUSIC &&
            __fb_sfx->music_playing >= 0)
        {
            __fb_sfx->music_pos = voice->position;
        }
    }

    return first_frame;
}

static void fb_sfxMixerFinishBlock(float *buffer, int frames, int use_simd)
{
    int frame;

    if (!buffer || frames <= 0)
        return;

    fb_sfxMidiSoftwareMixBlock(buffer, frames);
    fb_sfxEchoProcessBlock(buffer, frames);

#ifdef FB_SFX_HAS_SIMD
    if (use_simd)
    {
        fb_sfxClampFloatBufferSIMD(buffer, frames * 2);
        return;
    }
#else
    (void)use_simd;
#endif

    for (frame = 0; frame < frames * 2; frame++)
        buffer[frame] = fb_sfxMixerClamp(buffer[frame]);
}

void fb_sfxMixerProcess(int frames)
{
    float *buffer;
    float master_volume;
    float balance;
    int use_simd;
    int voice;

    if (!__fb_sfx || frames <= 0 || frames > INT_MAX / 2)
        return;

    buffer = __fb_sfx->mixbuffer;
    if (!buffer)
        return;

    /*
        rand() is process-wide state.  Rendering noise voices one complete
        block at a time would assign that shared random sequence to different
        voices than the established frame-major mixer.  Keep that uncommon
        case on the compatibility path until noise owns per-voice state.
    */
    if (fb_sfxMixerHasSharedNoise(frames))
    {
        fb_sfxMixerProcessFrameMajor(frames);
        return;
    }

    fb_sfxMixerClear(buffer, frames);
    master_volume = __fb_sfx->master_volume;
    balance = __fb_sfx->balance;
    use_simd = fb_sfxMixerSimdEnabled();

    /*
        Voice-major rendering turns 64 voice-slot checks per output frame into
        one bounded scan per block.  Runtime commands cannot mutate channel or
        voice controls during this call because the sound core holds the
        recursive runtime lock around mixer generation.
    */
    for (voice = 0; voice < FB_SFX_MAX_VOICES; voice++)
    {
        FB_SFXVOICE *v = &__fb_sfx->voices[voice];
        FB_SFX_MIXER_VOICE_GAINS gains;
        int first_frame;

        if (!v->active || v->paused)
            continue;

        fb_sfxMixerVoiceGains(v, master_volume, balance, &gains);
        first_frame = fb_sfxMixerConsumeStartDelay(v, frames);

        if (first_frame < frames && v->active)
        {
            first_frame = fb_sfxMixerRenderContiguousSample(v,
                                                            buffer,
                                                            frames,
                                                            first_frame,
                                                            &gains,
                                                            use_simd);
        }

        if (first_frame < frames && v->active)
        {
            first_frame = fb_sfxMixerRenderSustainedWaveform(v,
                                                             buffer,
                                                             frames,
                                                             first_frame,
                                                             &gains,
                                                             use_simd);
        }

        if (first_frame < frames && v->active)
        {
            first_frame = fb_sfxMixerRenderResampledVoice(v,
                                                          buffer,
                                                          frames,
                                                          first_frame,
                                                          &gains,
                                                          use_simd);
        }

        if (first_frame < frames && v->active)
        {
            fb_sfxMixerRenderVoiceScalar(v,
                                         buffer,
                                         frames,
                                         first_frame,
                                         &gains);
        }
    }

    fb_sfxMixerFinishBlock(buffer, frames, use_simd);
}


/* end of sfx_mixer.c */
