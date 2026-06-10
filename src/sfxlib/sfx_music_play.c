/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_play.c

    Purpose:

        Implement playback for the current MUSIC asset.

        MUSIC LOAD stores one decoded music file at a time. MUSIC PLAY
        and MUSIC LOOP start that current asset instead of selecting
        by numeric id.

    Responsibilities:

        • start music playback
        • manage music playback state
        • support loop configuration

    This file intentionally does NOT contain:

        • music decoding
        • audio synthesis
        • driver interaction
        • file-oriented MUSIC helpers
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Current music helpers                                                     */
/* ------------------------------------------------------------------------- */

static FB_SFXVOICE *fb_sfxCurrentMusicVoiceLocked(void)
{
    int i;

    if (!__fb_sfx)
        return NULL;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (voice->active && voice->type == FB_SFX_VOICE_MUSIC)
            return voice;
    }

    return NULL;
}

static float fb_sfxMusicSampleStepLocked(void)
{
    FB_SFX_ASSET *asset;

    if (!__fb_sfx)
        return 1.0f;

    asset = &__fb_sfx->music;

    if (asset->sample_rate <= 0 || __fb_sfx->samplerate <= 0)
        return 1.0f;

    return (float)asset->sample_rate / (float)__fb_sfx->samplerate;
}

static int fb_sfxStartMusicVoice(int loop)
{
    FB_SFXVOICE *voice;

    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();

    if (!__fb_sfx->music.loaded ||
        !__fb_sfx->music.data ||
        __fb_sfx->music.frames <= 0)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    fb_sfxMusicStopLocked();

    voice = fb_sfxVoiceAllocLocked();
    if (!voice)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    voice->type = FB_SFX_VOICE_MUSIC;
    voice->sfx_id = 0;
    voice->channel = 0;
    voice->volume = 1.0f;
    voice->data = __fb_sfx->music.data;
    voice->length = __fb_sfx->music.size / (int)sizeof(float);
    voice->position = 0;
    voice->pos = 0;
    voice->sample_pos = 0.0f;
    voice->sample_step = fb_sfxMusicSampleStepLocked();
    voice->loop = loop ? 1 : 0;
    voice->env_level = 1.0f;
    voice->env_state = FB_SFX_ENV_SUSTAIN;

    __fb_sfx->music_playing = 0;
    __fb_sfx->music_paused = 0;
    __fb_sfx->music_loop = loop ? 1 : 0;
    __fb_sfx->music_pos = 0;

    fb_sfxVoiceActivateLocked(voice);
    fb_sfxRuntimeUnlock();

    return 0;
}


/* ------------------------------------------------------------------------- */
/* MUSIC PLAY                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicPlay()

    Start playback of the current music asset.
*/

int fb_sfxMusicPlay(void)
{
    if (fb_sfxStartMusicVoice(0) != 0)
        return -1;

    SFX_DEBUG("sfx_music_play: current music started");
    return 0;
}


/* ------------------------------------------------------------------------- */
/* MUSIC LOOP                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicLoop()

    Start looping playback of the current music asset.
*/

int fb_sfxMusicLoop(void)
{
    if (fb_sfxStartMusicVoice(1) != 0)
        return -1;

    SFX_DEBUG("sfx_music_play: current music looping");
    return 0;
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
    FB_SFXVOICE *voice;

    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();

    if (__fb_sfx->music_playing < 0)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    voice = fb_sfxCurrentMusicVoiceLocked();
    __fb_sfx->music_pos = 0;

    if (voice)
    {
        voice->position = 0;
        voice->pos = 0;
        voice->sample_pos = 0.0f;
    }

    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_music_play: current music restarted");
}


/* end of sfx_music_play.c */
