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
    int voice;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_SFX)
        return;

    if (!__fb_sfx->sfx[id].loaded)
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
    __fb_sfx->voices[voice].channel = channel;
    __fb_sfx->voices[voice].volume = 1.0f;
    __fb_sfx->voices[voice].data = (const float *)__fb_sfx->sfx[id].data;
    __fb_sfx->voices[voice].length = __fb_sfx->sfx[id].size / (int)sizeof(float);
    __fb_sfx->voices[voice].env_level = 1.0f;
    __fb_sfx->voices[voice].env_state = FB_SFX_ENV_SUSTAIN;

    SFX_DEBUG(
        "sfx_sfx_play: id=%d voice=%d channel=%d",
        id,
        voice,
        channel
    );
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


/* ------------------------------------------------------------------------- */
/* SFX LOOP (channel)                                                        */
/* ------------------------------------------------------------------------- */

void fb_sfxSfxLoopChannel(int channel, int id)
{
    int voice;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_SFX)
        return;

    if (!__fb_sfx->sfx[id].loaded)
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
    __fb_sfx->voices[voice].channel = channel;
    __fb_sfx->voices[voice].volume = 1.0f;
    __fb_sfx->voices[voice].loop = 1;
    __fb_sfx->voices[voice].data = (const float *)__fb_sfx->sfx[id].data;
    __fb_sfx->voices[voice].length = __fb_sfx->sfx[id].size / (int)sizeof(float);
    __fb_sfx->voices[voice].env_level = 1.0f;
    __fb_sfx->voices[voice].env_state = FB_SFX_ENV_SUSTAIN;

    SFX_DEBUG(
        "sfx_sfx_play: loop id=%d voice=%d channel=%d",
        id,
        voice,
        channel
    );
}


/* end of sfx_sfx_play.c */
