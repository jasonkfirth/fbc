/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_play.c

    Purpose:

        Implement the MUSIC PLAY command.

        This activates playback of a music asset that was
        previously loaded with MUSIC LOAD.

    Responsibilities:

        • start music playback
        • manage music playback state
        • support loop configuration

    This file intentionally does NOT contain:

        • music decoding
        • audio synthesis
        • driver interaction
        • streaming logic

    Architectural overview:

        MUSIC PLAY
             │
             ▼
        playback state
             │
             ▼
        mixer/music subsystem
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

static FB_SFXVOICE *fb_sfxCurrentMusicVoice(void)
{
    int i;

    if (!__fb_sfx || __fb_sfx->music_playing < 0)
        return NULL;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (voice->active &&
            voice->type == FB_SFX_VOICE_MUSIC &&
            voice->sfx_id == __fb_sfx->music_playing)
            return voice;
    }

    return NULL;
}

static void fb_sfxStartMusicVoice(int id, int loop)
{
    FB_SFXVOICE *voice;

    fb_sfxMusicStop();

    voice = fb_sfxVoiceAlloc();
    if (!voice)
        return;

    voice->type = FB_SFX_VOICE_MUSIC;
    voice->sfx_id = id;
    voice->channel = 0;
    voice->volume = 1.0f;
    voice->data = (const float *)__fb_sfx->music[id].data;
    voice->length = __fb_sfx->music[id].size / (int)sizeof(float);
    voice->position = 0;
    voice->loop = loop ? 1 : 0;
    voice->env_level = 1.0f;
    voice->env_state = FB_SFX_ENV_SUSTAIN;

    __fb_sfx->music_playing = id;
    __fb_sfx->music_paused = 0;
    __fb_sfx->music_loop = loop ? 1 : 0;
    __fb_sfx->music_pos = 0;
}


/* ------------------------------------------------------------------------- */
/* MUSIC PLAY                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicPlay()

    Start playback of a loaded music asset.

    Parameters:

        id    music identifier returned by MUSIC LOAD
*/

void fb_sfxMusicPlay(int id)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_MUSIC)
        return;

    if (!__fb_sfx->music[id].loaded)
        return;

    fb_sfxStartMusicVoice(id, 0);

    SFX_DEBUG(
        "sfx_music_play: id=%d started",
        id
    );
}


/* ------------------------------------------------------------------------- */
/* MUSIC LOOP                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicLoop()

    Start looping playback of a loaded music asset.
*/

void fb_sfxMusicLoop(int id)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_MUSIC)
        return;

    if (!__fb_sfx->music[id].loaded)
        return;

    fb_sfxStartMusicVoice(id, 1);

    SFX_DEBUG(
        "sfx_music_play: id=%d looping",
        id
    );
}


/* ------------------------------------------------------------------------- */
/* MUSIC RESTART                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicRestart()

    Restart currently playing music from the beginning.
*/

void fb_sfxMusicRestart(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (__fb_sfx->music_playing < 0)
        return;

    {
        FB_SFXVOICE *voice = fb_sfxCurrentMusicVoice();

        __fb_sfx->music_pos = 0;

        if (voice)
            voice->position = 0;
    }

    SFX_DEBUG(
        "sfx_music_play: restart id=%d",
        __fb_sfx->music_playing
    );
}


/* end of sfx_music_play.c */
