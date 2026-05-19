/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_sfx_play.c

    Purpose:

        Implement the SFX PLAY command.

        This triggers playback of a loaded sound effect by
        allocating a voice and routing it through the mixer.

    Responsibilities:

        • validate SFX identifiers
        • allocate playback voices
        • route SFX playback through channels

    This file intentionally does NOT contain:

        • audio decoding
        • mixer algorithms
        • driver interaction

    Architectural overview:

        SFX PLAY
             │
             ▼
        voice allocation
             │
             ▼
        channel routing
             │
             ▼
        mixer
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Pitch limits                                                              */
/* ------------------------------------------------------------------------- */

/* Four octaves down keeps fractional stepping useful without stalling. */
#define FB_SFX_MIN_SAMPLE_PITCH 0.0625f

/* Four octaves up keeps the mixer from skipping huge spans of memory. */
#define FB_SFX_MAX_SAMPLE_PITCH 16.0f


/* ------------------------------------------------------------------------- */
/* Internal helper                                                           */
/* ------------------------------------------------------------------------- */

static int fb_sfxFindFreeVoice(void)
{
    int i;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (!__fb_sfx->voices[i].active)
            return i;
    }

    /* fallback: steal oldest voice */

    return 0;
}

static float fb_sfxSfxPitchStep(const FB_SFX_ASSET *asset, float pitch)
{
    float step;

    if (pitch <= 0.0f)
        pitch = 1.0f;

    if (pitch < FB_SFX_MIN_SAMPLE_PITCH)
        pitch = FB_SFX_MIN_SAMPLE_PITCH;

    if (pitch > FB_SFX_MAX_SAMPLE_PITCH)
        pitch = FB_SFX_MAX_SAMPLE_PITCH;

    step = pitch;

    /*
        Loaded samples keep their source sample rate.  The mixer
        advances by a fractional amount so a 22050 Hz effect still
        plays at the right speed on a 44100 or 48000 Hz device.
    */

    if (asset &&
        asset->sample_rate > 0 &&
        __fb_sfx &&
        __fb_sfx->samplerate > 0)
    {
        step *= (float)asset->sample_rate / (float)__fb_sfx->samplerate;
    }

    if (step <= 0.0f)
        step = 1.0f;

    return step;
}

static void fb_sfxSfxStart(int channel, int id, float pitch, int loop)
{
    int voice;
    FB_SFX_ASSET *asset;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_SFX)
        return;

    asset = &__fb_sfx->sfx[id];

    if (!asset->loaded)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        channel = 0;

    voice = fb_sfxFindFreeVoice();

    fb_sfxVoiceInit(&__fb_sfx->voices[voice]);
    __fb_sfx->voices[voice].active = 1;
    __fb_sfx->voices[voice].type = FB_SFX_VOICE_SFX;
    __fb_sfx->voices[voice].sfx_id = id;
    __fb_sfx->voices[voice].position = 0;
    __fb_sfx->voices[voice].pos = 0;
    __fb_sfx->voices[voice].sample_pos = 0.0f;
    __fb_sfx->voices[voice].sample_step = fb_sfxSfxPitchStep(asset, pitch);
    __fb_sfx->voices[voice].channel = channel;
    __fb_sfx->voices[voice].volume = 1.0f;
    __fb_sfx->voices[voice].loop = loop ? 1 : 0;
    __fb_sfx->voices[voice].data = asset->data;
    __fb_sfx->voices[voice].length = asset->size / (int)sizeof(float);
    __fb_sfx->voices[voice].env_level = 1.0f;
    __fb_sfx->voices[voice].env_state = FB_SFX_ENV_SUSTAIN;

    SFX_DEBUG(
        "sfx_sfx_play: id=%d voice=%d channel=%d pitch=%f loop=%d step=%f",
        id,
        voice,
        channel,
        pitch,
        loop ? 1 : 0,
        __fb_sfx->voices[voice].sample_step
    );
}


/* ------------------------------------------------------------------------- */
/* SFX PLAY                                                                  */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxPlay()

    Play a sound effect on the current channel.
*/

void fb_sfxSfxPlay(int id)
{
    int channel;

    if (!fb_sfxEnsureInitialized())
        return;

    channel = __fb_sfx->current_channel;

    fb_sfxSfxPlayChannel(channel, id);
}

void fb_sfxSfxPlayPitch(int id, float pitch)
{
    int channel;

    if (!fb_sfxEnsureInitialized())
        return;

    channel = __fb_sfx->current_channel;

    fb_sfxSfxPlayChannelPitch(channel, id, pitch);
}


/* ------------------------------------------------------------------------- */
/* SFX PLAY (channel)                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxPlayChannel()

    Play a sound effect on a specific channel.

    Parameters:

        channel  target channel
        id       SFX identifier
*/

void fb_sfxSfxPlayChannel(int channel, int id)
{
    fb_sfxSfxStart(channel, id, 1.0f, 0);
}

void fb_sfxSfxPlayChannelPitch(int channel, int id, float pitch)
{
    fb_sfxSfxStart(channel, id, pitch, 0);
}


/* ------------------------------------------------------------------------- */
/* SFX LOOP                                                                  */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxLoop()

    Play a looping sound effect.
*/

void fb_sfxSfxLoop(int id)
{
    int channel;

    if (!fb_sfxEnsureInitialized())
        return;

    channel = __fb_sfx->current_channel;

    fb_sfxSfxLoopChannel(channel, id);
}

void fb_sfxSfxLoopPitch(int id, float pitch)
{
    int channel;

    if (!fb_sfxEnsureInitialized())
        return;

    channel = __fb_sfx->current_channel;

    fb_sfxSfxLoopChannelPitch(channel, id, pitch);
}


/* ------------------------------------------------------------------------- */
/* SFX LOOP (channel)                                                        */
/* ------------------------------------------------------------------------- */

void fb_sfxSfxLoopChannel(int channel, int id)
{
    fb_sfxSfxStart(channel, id, 1.0f, 1);
}

void fb_sfxSfxLoopChannelPitch(int channel, int id, float pitch)
{
    fb_sfxSfxStart(channel, id, pitch, 1);
}


/* end of sfx_sfx_play.c */
