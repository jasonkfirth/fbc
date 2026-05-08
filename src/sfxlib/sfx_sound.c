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

#include <math.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Default parameters                                                        */
/* ------------------------------------------------------------------------- */

#define FB_SFX_SOUND_VOLUME 0.8f
#define FB_SFX_AMIGA_TICKS_PER_SECOND 18.2f
#define FB_SFX_BBC_TICKS_PER_SECOND 20.0f
#define FB_SFX_C128_TICKS_PER_SECOND 60.0f
#define FB_SFX_C128_CLOCK 1022727.0f
#define FB_SFX_SID_PHASE_SCALE 16777216.0f
#define FB_SFX_TI_MILLISECONDS_PER_SECOND 1000.0f
#define FB_SFX_ATARI_CLOCK 31960.0f

static void fb_sfxSoundQueue(
    int channel,
    int frequency,
    float duration,
    float volume,
    int waveform,
    int start_delay);
static int fb_sfxSoundDurationFrames(float duration);
static int fb_sfxSoundClampInt(int value, int min_value, int max_value);
static float fb_sfxSoundClampVolume(float volume);
static int fb_sfxSoundC128Frequency(int freq);
static int fb_sfxSoundC128Waveform(int waveform);
static void fb_sfxSoundC128(int voice,
                            int freq,
                            int duration,
                            int direction,
                            int minimum,
                            int step_value,
                            int waveform,
                            int pulse_width);
static void fb_sfxSoundAmiga(int frequency, int duration, int volume, int voice);
static void fb_sfxSoundBbc(int channel, int amplitude, int pitch, int duration, int after);
static void fb_sfxSoundAtari(int voice, int pitch, int distortion, int volume);
static void fb_sfxSoundClassicPc(int frequency, int duration);
static int fb_sfxSoundTiPairLooksValid(int frequency, int volume);
static void fb_sfxSoundTi3(int duration, int frequency, int volume);
static void fb_sfxSoundTi5(int duration,
                           int frequency1,
                           int volume1,
                           int frequency2,
                           int volume2);
static void fb_sfxSoundTi7(int duration,
                           int frequency1,
                           int volume1,
                           int frequency2,
                           int volume2,
                           int frequency3,
                           int volume3);


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
    if (!fb_sfxEnsureInitialized())
        return;

    if (frequency <= 0)
        return;

    if (duration <= 0.0f)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        channel = 0;

    volume = fb_sfxSoundClampVolume(volume);
    fb_sfxSoundQueue(channel, frequency, duration, volume, FB_SFX_WAVE_TRIANGLE, 0);

    SFX_DEBUG(
        "sfx_sound: ch=%d freq=%d dur=%f vol=%f",
        channel,
        frequency,
        duration,
        volume
    );
}


/* ------------------------------------------------------------------------- */
/* Legacy BASIC SOUND dispatch                                               */
/* ------------------------------------------------------------------------- */

/*
    These entry points are intentionally narrow compiler overload targets for
    old BASIC listings that used integer durations or dialect-specific channel
    arguments.  In the ambiguous two-argument form, Microsoft PC BASIC wins:
    BASICA, GW-BASIC, QBasic 1.1, QuickBASIC 4.5 and PDS 7.1 all use
    SOUND freq,duration with duration measured in timer ticks.
*/

void fb_sfxSoundLegacy2(int a, int b)
{
    fb_sfxSoundClassicPc(a, b);
}

void fb_sfxSoundLegacy3(int a, int b, int c)
{
    if (a >= 1 && a <= 3)
    {
        fb_sfxSoundC128(a, b, c, 0, 0, 0, 2, 0);
        return;
    }

    if (a >= -4250 && a <= 4250 && fb_sfxSoundTiPairLooksValid(b, c))
    {
        fb_sfxSoundTi3(a, b, c);
        return;
    }

    fb_sfxSoundAmiga(a, b, c, 0);
}

void fb_sfxSoundLegacy4(int a, int b, int c, int d)
{
    if (b <= 0)
    {
        fb_sfxSoundBbc(a, b, c, d, 0);
        return;
    }

    if (a >= 1 && a <= 3 && c > 4 && d >= 0 && d <= 2)
    {
        fb_sfxSoundC128(a, b, c, d, 0, 0, 2, 0);
        return;
    }

    if (a >= 0 && a <= 3 && c >= 0 && c <= 15 && d >= 0 && d <= 15)
    {
        fb_sfxSoundAtari(a, b, c, d);
        return;
    }

    if (a >= 20)
    {
        fb_sfxSoundAmiga(a, b, c, d);
        return;
    }

    fb_sfxSoundChannel(a, b, (float)c, (float)d);
}

void fb_sfxSoundLegacy5(int a, int b, int c, int d, int e)
{
    if (b <= 0)
    {
        fb_sfxSoundBbc(a, b, c, d, e);
        return;
    }

    if (a >= -4250 && a <= 4250 &&
        fb_sfxSoundTiPairLooksValid(b, c) &&
        fb_sfxSoundTiPairLooksValid(d, e))
    {
        fb_sfxSoundTi5(a, b, c, d, e);
        return;
    }

    if (a >= 1 && a <= 3)
        fb_sfxSoundC128(a, b, c, d, e, 0, 2, 0);
}

void fb_sfxSoundLegacy6(int a, int b, int c, int d, int e, int f)
{
    if (a >= 1 && a <= 3)
        fb_sfxSoundC128(a, b, c, d, e, f, 2, 0);
}

void fb_sfxSoundLegacy7(int a, int b, int c, int d, int e, int f, int g)
{
    if (a >= -4250 && a <= 4250 &&
        fb_sfxSoundTiPairLooksValid(b, c) &&
        fb_sfxSoundTiPairLooksValid(d, e) &&
        fb_sfxSoundTiPairLooksValid(f, g))
    {
        fb_sfxSoundTi7(a, b, c, d, e, f, g);
        return;
    }

    if (a >= 1 && a <= 3)
        fb_sfxSoundC128(a, b, c, d, e, f, g, 0);
}

void fb_sfxSoundLegacy8(int a, int b, int c, int d, int e, int f, int g, int h)
{
    if (a >= 1 && a <= 3)
        fb_sfxSoundC128(a, b, c, d, e, f, g, h);
}


/* ------------------------------------------------------------------------- */
/* Legacy BASIC helpers                                                      */
/* ------------------------------------------------------------------------- */

static void fb_sfxSoundQueue(
    int channel,
    int frequency,
    float duration,
    float volume,
    int waveform,
    int start_delay)
{
    FB_SFXVOICE *voice;

    voice = fb_sfxVoiceAlloc();
    if (!voice)
        return;

    voice->type = FB_SFX_VOICE_SOUND;
    voice->channel = channel;
    voice->volume = fb_sfxSoundClampVolume(volume);
    voice->start_delay = start_delay;
    voice->hard_stop = 1;

    fb_sfxInstrumentApply(voice, channel, waveform, 0);
    fb_sfxVoiceSetFrequency(voice, frequency);

    voice->length = fb_sfxSoundDurationFrames(duration);
    voice->position = 0;
}

static int fb_sfxSoundDurationFrames(float duration)
{
    int frames;

    if (!__fb_sfx || __fb_sfx->samplerate <= 0 || duration <= 0.0f)
        return 0;

    frames = (int)(duration * (float)__fb_sfx->samplerate + 0.5f);
    if (frames <= 0)
        frames = 1;

    return frames;
}

static int fb_sfxSoundClampInt(int value, int min_value, int max_value)
{
    if (value < min_value)
        return min_value;

    if (value > max_value)
        return max_value;

    return value;
}

static float fb_sfxSoundClampVolume(float volume)
{
    if (volume < 0.0f)
        return 0.0f;

    if (volume > 1.0f)
        return 1.0f;

    return volume;
}

static int fb_sfxSoundC128Frequency(int freq)
{
    float hz;

    freq = fb_sfxSoundClampInt(freq, 0, 65535);

    hz = ((float)freq * FB_SFX_C128_CLOCK) / FB_SFX_SID_PHASE_SCALE;

    if (hz < 20.0f)
        hz = 20.0f;

    if (hz > 15000.0f)
        hz = 15000.0f;

    return (int)(hz + 0.5f);
}

static int fb_sfxSoundC128Waveform(int waveform)
{
    switch (waveform)
    {
        case 0:
            return FB_SFX_WAVE_TRIANGLE;

        case 1:
            return FB_SFX_WAVE_SAW;

        case 3:
            return FB_SFX_WAVE_NOISE;
    }

    return FB_SFX_WAVE_SQUARE;
}

static void fb_sfxSoundC128(int voice,
                            int freq,
                            int duration,
                            int direction,
                            int minimum,
                            int step_value,
                            int waveform,
                            int pulse_width)
{
    int channel;
    int total_frames;
    int segment_frames;
    int start_delay;
    int current;
    int target;
    int step;
    int sweep_down;
    int wave;
    float seconds;
    float duration_this_pass;

    (void)pulse_width;

    if (!fb_sfxEnsureInitialized())
        return;

    voice = fb_sfxSoundClampInt(voice, 1, 3);
    channel = voice - 1;

    if (duration <= 0)
    {
        fb_sfxSoundStopChannel(channel);
        return;
    }

    seconds = (float)duration / FB_SFX_C128_TICKS_PER_SECOND;
    total_frames = fb_sfxSoundDurationFrames(seconds);
    if (total_frames <= 0)
        return;

    direction = fb_sfxSoundClampInt(direction, 0, 2);
    freq = fb_sfxSoundClampInt(freq, 0, 65535);
    minimum = fb_sfxSoundClampInt(minimum, 0, 65535);
    if (step_value < 0)
        step_value = 0;

    wave = fb_sfxSoundC128Waveform(waveform);

    if (step_value == 0 || freq == minimum)
    {
        fb_sfxSoundQueue(channel,
                         fb_sfxSoundC128Frequency(freq),
                         seconds,
                         FB_SFX_SOUND_VOLUME,
                         wave,
                         0);
        fb_sfxRunForeground(total_frames);
        return;
    }

    segment_frames = (__fb_sfx->samplerate > 0)
        ? (__fb_sfx->samplerate / 10)
        : 1470;
    if (segment_frames <= 0)
        segment_frames = 1;

    current = freq;
    target = minimum;
    step = (step_value > 0) ? step_value : 1;
    sweep_down = 1;
    start_delay = 0;

    while (start_delay < total_frames)
    {
        int frames_left;
        int this_frames;

        frames_left = total_frames - start_delay;
        this_frames = (frames_left > segment_frames) ? segment_frames : frames_left;
        duration_this_pass = (__fb_sfx->samplerate > 0)
            ? (float)this_frames / (float)__fb_sfx->samplerate
            : (float)this_frames / (float)FB_SFX_DEFAULT_RATE;

        fb_sfxSoundQueue(channel,
                         fb_sfxSoundC128Frequency(current),
                         duration_this_pass,
                         FB_SFX_SOUND_VOLUME,
                         wave,
                         start_delay);

        if (direction == 1)
        {
            current -= step;
            if (current < target)
                current = target;
        }
        else if (direction == 2)
        {
            if (sweep_down)
            {
                current -= step;
                if (current <= target)
                {
                    current = target;
                    sweep_down = 0;
                }
            }
            else
            {
                current += step;
                if (current >= freq)
                {
                    current = freq;
                    sweep_down = 1;
                }
            }
        }
        else
        {
            current += step;
            if (current > freq)
                current = minimum;
        }

        start_delay += this_frames;
    }

    fb_sfxRunForeground(total_frames);
}

static void fb_sfxSoundAmiga(int frequency, int duration, int volume, int voice)
{
    int channel;
    int frames;
    float seconds;
    float level;

    if (!fb_sfxEnsureInitialized())
        return;

    frequency = fb_sfxSoundClampInt(frequency, 20, 15000);
    duration = fb_sfxSoundClampInt(duration, 0, 77);
    volume = fb_sfxSoundClampInt(volume, 0, 255);
    voice = fb_sfxSoundClampInt(voice, 0, 3);

    if (duration == 0)
    {
        fb_sfxSoundStopChannel(voice);
        return;
    }

    channel = voice;
    seconds = (float)duration / FB_SFX_AMIGA_TICKS_PER_SECOND;
    level = (float)volume / 255.0f;

    fb_sfxSoundQueue(channel, frequency, seconds, level, FB_SFX_WAVE_TRIANGLE, 0);

    frames = fb_sfxSoundDurationFrames(seconds);
    fb_sfxRunForeground(frames);
}

static void fb_sfxSoundBbc(int channel, int amplitude, int pitch, int duration, int after)
{
    int frames;
    int start_delay;
    float seconds;
    float volume;
    double frequency;

    if (!fb_sfxEnsureInitialized())
        return;

    channel = fb_sfxSoundClampInt(channel, 1, 8) - 1;
    pitch = fb_sfxSoundClampInt(pitch, 0, 255);
    duration = fb_sfxSoundClampInt(duration, 0, 32767);

    if (duration == 255)
        duration = 254;

    seconds = (float)duration / FB_SFX_BBC_TICKS_PER_SECOND;
    volume = (float)fb_sfxSoundClampInt(-amplitude, 0, 15) / 15.0f;
    frequency = 261.625565 * pow(2.0, (double)(pitch - 53) / 48.0);

    start_delay = 0;
    if (after > 0 && __fb_sfx && __fb_sfx->samplerate > 0)
        start_delay = (int)(((float)after / FB_SFX_BBC_TICKS_PER_SECOND) * (float)__fb_sfx->samplerate);

    fb_sfxSoundQueue(channel,
                     (int)(frequency + 0.5),
                     seconds,
                     volume,
                     FB_SFX_WAVE_SQUARE,
                     start_delay);

    frames = start_delay + fb_sfxSoundDurationFrames(seconds);
    fb_sfxRunForeground(frames);
}

static void fb_sfxSoundAtari(int voice, int pitch, int distortion, int volume)
{
    int channel;
    int frequency;
    float level;
    float seconds;

    if (!fb_sfxEnsureInitialized())
        return;

    channel = fb_sfxSoundClampInt(voice, 0, 3);
    pitch = fb_sfxSoundClampInt(pitch, 0, 255);
    distortion = fb_sfxSoundClampInt(distortion, 0, 15);
    volume = fb_sfxSoundClampInt(volume, 0, 15);

    if (pitch == 0 || distortion == 0 || volume == 0)
    {
        fb_sfxSoundStopChannel(channel);
        return;
    }

    frequency = (int)(FB_SFX_ATARI_CLOCK / (float)pitch + 0.5f);
    if (frequency < 20)
        frequency = 20;
    if (frequency > 15000)
        frequency = 15000;

    level = (float)volume / 15.0f;
    seconds = 1.0f / 30.0f;

    fb_sfxSoundQueue(channel, frequency, seconds, level, FB_SFX_WAVE_SQUARE, 0);
    fb_sfxRunForeground(fb_sfxSoundDurationFrames(seconds));
}

static int fb_sfxSoundTiPairLooksValid(int frequency, int volume)
{
    if (volume < 0 || volume > 30)
        return 0;

    if (frequency >= 110 && frequency <= 44733)
        return 1;

    if (frequency >= -8 && frequency <= -1)
        return 1;

    return 0;
}

static void fb_sfxSoundTiPair(int channel,
                              int frequency,
                              int volume,
                              float seconds)
{
    float level;

    level = (float)(30 - fb_sfxSoundClampInt(volume, 0, 30)) / 30.0f;
    level *= FB_SFX_SOUND_VOLUME;

    if (frequency >= -8 && frequency <= -1)
    {
        fb_sfxSoundQueue(channel, 0, seconds, level, FB_SFX_WAVE_NOISE, 0);
        return;
    }

    frequency = fb_sfxSoundClampInt(frequency, 20, 15000);
    fb_sfxSoundQueue(channel, frequency, seconds, level, FB_SFX_WAVE_SQUARE, 0);
}

static void fb_sfxSoundTiRun(int duration, int channels_used)
{
    int frames;
    float seconds;

    if (!fb_sfxEnsureInitialized())
        return;

    if (duration == 0 || channels_used <= 0)
        return;

    if (duration < 0)
        duration = -duration;

    duration = fb_sfxSoundClampInt(duration, 1, 4250);
    seconds = (float)duration / FB_SFX_TI_MILLISECONDS_PER_SECOND;
    frames = fb_sfxSoundDurationFrames(seconds);

    fb_sfxRunForeground(frames);
}

static void fb_sfxSoundTi3(int duration, int frequency, int volume)
{
    float seconds;

    if (!fb_sfxEnsureInitialized())
        return;

    if (duration < 0)
        duration = -duration;

    duration = fb_sfxSoundClampInt(duration, 1, 4250);
    seconds = (float)duration / FB_SFX_TI_MILLISECONDS_PER_SECOND;

    fb_sfxSoundTiPair(0, frequency, volume, seconds);
    fb_sfxSoundTiRun(duration, 1);
}

static void fb_sfxSoundTi5(int duration,
                           int frequency1,
                           int volume1,
                           int frequency2,
                           int volume2)
{
    float seconds;

    if (!fb_sfxEnsureInitialized())
        return;

    if (duration < 0)
        duration = -duration;

    duration = fb_sfxSoundClampInt(duration, 1, 4250);
    seconds = (float)duration / FB_SFX_TI_MILLISECONDS_PER_SECOND;

    fb_sfxSoundTiPair(0, frequency1, volume1, seconds);
    fb_sfxSoundTiPair(1, frequency2, volume2, seconds);
    fb_sfxSoundTiRun(duration, 2);
}

static void fb_sfxSoundTi7(int duration,
                           int frequency1,
                           int volume1,
                           int frequency2,
                           int volume2,
                           int frequency3,
                           int volume3)
{
    float seconds;

    if (!fb_sfxEnsureInitialized())
        return;

    if (duration < 0)
        duration = -duration;

    duration = fb_sfxSoundClampInt(duration, 1, 4250);
    seconds = (float)duration / FB_SFX_TI_MILLISECONDS_PER_SECOND;

    fb_sfxSoundTiPair(0, frequency1, volume1, seconds);
    fb_sfxSoundTiPair(1, frequency2, volume2, seconds);
    fb_sfxSoundTiPair(2, frequency3, volume3, seconds);
    fb_sfxSoundTiRun(duration, 3);
}

static void fb_sfxSoundClassicPc(int frequency, int duration)
{
    int frames;
    float seconds;

    if (!fb_sfxEnsureInitialized())
        return;

    frequency = fb_sfxSoundClampInt(frequency, 37, 32767);

    if (duration <= 0)
    {
        fb_sfxSoundStop();
        return;
    }

    duration = fb_sfxSoundClampInt(duration, 1, 65535);
    seconds = (float)duration / FB_SFX_AMIGA_TICKS_PER_SECOND;

    if (frequency < 20000)
        fb_sfxSoundQueue(0, frequency, seconds, FB_SFX_SOUND_VOLUME, FB_SFX_WAVE_SQUARE, 0);

    frames = fb_sfxSoundDurationFrames(seconds);
    fb_sfxRunForeground(frames);
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
