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

    if (__fb_sfx->music_playing < 0)
        return;

    if (!__fb_sfx->music_paused)
        return;

    __fb_sfx->music_paused = 0;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (voice->active &&
            voice->type == FB_SFX_VOICE_MUSIC &&
            voice->sfx_id == __fb_sfx->music_playing)
            voice->paused = 0;
    }

    SFX_DEBUG(
        "sfx_music_resume: id=%d resumed",
        __fb_sfx->music_playing
    );
}


/* ------------------------------------------------------------------------- */
/* MUSIC RESUME (specific id)                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicResumeId()

    Resume a specific music asset if it is currently paused.
*/

void fb_sfxMusicResumeId(int id)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_MUSIC)
        return;

    if (__fb_sfx->music_playing != id)
        return;

    fb_sfxMusicResume();
}


/* end of sfx_music_resume.c */
