/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_status.c

    Purpose:

        Implement the MUSIC STATUS command.

        This allows applications to query the current
        music playback state.

    Responsibilities:

        • report whether music is playing, paused, or stopped
        • expose current music asset identifier
        • provide helper queries for internal subsystems

    This file intentionally does NOT contain:

        • music playback logic
        • mixer processing
        • audio driver interaction
        • file loading

    Architectural overview:

        MUSIC STATUS
             │
             ▼
        playback state query
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Status values                                                             */
/* ------------------------------------------------------------------------- */

#define FB_SFX_MUSIC_STOPPED 0
#define FB_SFX_MUSIC_PLAYING 1
#define FB_SFX_MUSIC_PAUSED  2


/* ------------------------------------------------------------------------- */
/* MUSIC STATUS                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicStatus()

    Return the current music playback status.

    Returns:

        FB_SFX_MUSIC_STOPPED
        FB_SFX_MUSIC_PLAYING
        FB_SFX_MUSIC_PAUSED
*/

int fb_sfxMusicStatus(void)
{
    int i;

    if (!__fb_sfx)
        return FB_SFX_MUSIC_STOPPED;

    if (__fb_sfx->music_playing >= 0)
    {
        for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
        {
            FB_SFXVOICE *voice = &__fb_sfx->voices[i];

            if (voice->active &&
                voice->type == FB_SFX_VOICE_MUSIC &&
                voice->sfx_id == __fb_sfx->music_playing)
                break;
        }

        if (i >= FB_SFX_MAX_VOICES)
        {
            __fb_sfx->music_playing = -1;
            __fb_sfx->music_paused = 0;
            __fb_sfx->music_loop = 0;
            __fb_sfx->music_pos = 0;
            return FB_SFX_MUSIC_STOPPED;
        }
    }

    if (__fb_sfx->music_playing < 0)
        return FB_SFX_MUSIC_STOPPED;

    if (__fb_sfx->music_paused)
        return FB_SFX_MUSIC_PAUSED;

    return FB_SFX_MUSIC_PLAYING;
}


/* ------------------------------------------------------------------------- */
/* MUSIC CURRENT                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicCurrent()

    Return the identifier of the currently active music asset.

    Returns:

        music id or -1 if none is playing
*/

int fb_sfxMusicCurrent(void)
{
    if (!__fb_sfx)
        return -1;

    return __fb_sfx->music_playing;
}


/* ------------------------------------------------------------------------- */
/* MUSIC POSITION                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicPosition()

    Return the current playback position.

    The interpretation of this value depends on the
    music decoder implementation.
*/

long fb_sfxMusicPosition(void)
{
    if (!__fb_sfx)
        return 0;

    return __fb_sfx->music_pos;
}


/* end of sfx_music_status.c */
