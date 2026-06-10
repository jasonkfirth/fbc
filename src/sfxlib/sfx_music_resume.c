/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_resume.c

    Purpose:

        Implement the MUSIC RESUME command.

        This resumes music playback that was previously
        paused using MUSIC PAUSE.

    Responsibilities:

        • resume paused music playback
        • preserve playback position
        • update playback state flags

    This file intentionally does NOT contain:

        • music decoding
        • audio synthesis
        • streaming logic
        • driver interaction

    Architectural overview:

        MUSIC RESUME
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
/* MUSIC RESUME                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicResume()

    Resume playback of paused music.
*/

void fb_sfxMusicResume(void)
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

    if (!__fb_sfx->music_paused)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    __fb_sfx->music_paused = 0;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (voice->active &&
            voice->type == FB_SFX_VOICE_MUSIC)
            voice->paused = 0;
    }

    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_music_resume: current music resumed");
}


/* end of sfx_music_resume.c */
