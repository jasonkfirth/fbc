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
#define FB_SFX_PC_TICKS_PER_SECOND 18.2f

static int fb_sfxSoundDurationFrames(float duration);
static int fb_sfxSoundClampInt(int value, int min_value, int max_value);
static float fb_sfxSoundClampVolume(float volume);
static void fb_sfxSoundClassicPc(int frequency, int duration);


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
    This narrow compatibility entry point accepts the common Microsoft PC
    BASIC form:

        SOUND frequency, duration

    The duration is measured in timer ticks, matching BASICA, GW-BASIC,
    QBasic 1.1, QuickBASIC 4.5 and PDS 7.1.
*/

void fb_sfxSoundLegacy2(int a, int b)
{
    fb_sfxSoundClassicPc(a, b);
}

void fb_sfxSoundQueue(
    int channel,
    int frequency,
    float duration,
    float volume,
    int waveform,
    int start_delay)
{
    FB_SFXVOICE *voice;

    fb_sfxRuntimeLock();

    voice = fb_sfxVoiceAllocLocked();
    if (!voice)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    voice->type = FB_SFX_VOICE_SOUND;
    voice->channel = channel;
    voice->volume = fb_sfxSoundClampVolume(volume);
    voice->start_delay = start_delay;
    voice->hard_stop = 1;

    fb_sfxInstrumentApply(voice, channel, waveform, 0);
    fb_sfxVoiceSetFrequency(voice, frequency);

    voice->length = fb_sfxSoundDurationFrames(duration);
    voice->position = 0;

    fb_sfxVoiceActivateLocked(voice);
    fb_sfxRuntimeUnlock();
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
    seconds = (float)duration / FB_SFX_PC_TICKS_PER_SECOND;

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
