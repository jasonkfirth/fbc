/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_stop.c

    Purpose:

        Implement the MUSIC STOP command.

        This terminates music playback and resets the
        music playback state so the mixer no longer
        attempts to process music frames.

    Responsibilities:

        • stop active music playback
        • reset playback position
        • clear loop and pause state

    This file intentionally does NOT contain:

        • music decoding
        • streaming logic
        • audio driver interaction
        • file management

    Architectural overview:

        MUSIC STOP
             │
             ▼
        playback state reset
             │
             ▼
        mixer/music subsystem
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* MUSIC STOP                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicStop()

    Stop currently playing music.
*/

void fb_sfxMusicStop(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    if (__fb_sfx->music_playing < 0)
        return;

    SFX_DEBUG(
        "sfx_music_stop: id=%d stopped",
        __fb_sfx->music_playing
    );

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (voice->type == FB_SFX_VOICE_MUSIC)
            voice->active = 0;
    }

    __fb_sfx->music_playing = -1;
    __fb_sfx->music_paused  = 0;
    __fb_sfx->music_loop    = 0;
    __fb_sfx->music_pos     = 0;
}


/* ------------------------------------------------------------------------- */
/* MUSIC STOP (specific id)                                                  */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicStopId()

    Stop a specific music asset if it is currently playing.
*/

void fb_sfxMusicStopId(int id)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_MUSIC)
        return;

    if (__fb_sfx->music_playing != id)
        return;

    fb_sfxMusicStop();
}


/* end of sfx_music_stop.c */
