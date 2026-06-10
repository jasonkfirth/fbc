/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_pause.c

    Purpose:

        Implement the MUSIC PAUSE command.

        This suspends music playback without resetting the
        playback position, allowing playback to resume later.

    Responsibilities:

        • pause active music playback
        • preserve playback position
        • update playback state flags

    This file intentionally does NOT contain:

        • music decoding
        • audio synthesis
        • streaming logic
        • driver interaction

    Architectural overview:

        MUSIC PAUSE
             │
             ▼
        playback state update
             │
             ▼
        mixer/music subsystem
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* MUSIC PAUSE                                                               */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicPause()

    Pause currently playing music.
*/

void fb_sfxMusicPause(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();

    if (__fb_sfx->music_playing < 0)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    if (__fb_sfx->music_paused)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    __fb_sfx->music_paused = 1;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (voice->active &&
            voice->type == FB_SFX_VOICE_MUSIC)
            voice->paused = 1;
    }

    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_music_pause: current music paused");
}


/* end of sfx_music_pause.c */
