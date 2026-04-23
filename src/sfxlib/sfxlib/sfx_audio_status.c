/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_audio_status.c

    Purpose:

        Implement the AUDIO STATUS command.

        This command reports the current state of the audio
        streaming subsystem used by AUDIO PLAY.

    Responsibilities:

        • report whether audio playback is active
        • report whether playback is paused
        • provide a simple status query interface for BASIC programs

    This file intentionally does NOT contain:

        • audio decoding logic
        • mixer algorithms
        • driver interaction
        • stream initialization logic

    Architectural overview:

        AUDIO STATUS
              │
        query streaming state
              │
        return playback status
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* External stream state                                                     */
/* ------------------------------------------------------------------------- */

/*
    The audio stream playback state is maintained by the
    streaming subsystem implemented in sfx_audio_play.c.
*/

extern int g_audio_playing;
extern int g_audio_paused;


/* ------------------------------------------------------------------------- */
/* Status constants                                                          */
/* ------------------------------------------------------------------------- */

/*
    These values provide a stable interface for both internal
    code and BASIC programs querying the streaming state.
*/

#define FB_SFX_AUDIO_STOPPED 0
#define FB_SFX_AUDIO_PLAYING 1
#define FB_SFX_AUDIO_PAUSED  2


/* ------------------------------------------------------------------------- */
/* AUDIO STATUS                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioStatus()

    Return the current playback status of the audio stream.

    Return values:

        0 = stopped
        1 = playing
        2 = paused
*/

int fb_sfxAudioStatus(void)
{
    int status;

    fb_sfxRuntimeLock();
    if (!g_audio_playing)
    {
        fb_sfxRuntimeUnlock();
        return FB_SFX_AUDIO_STOPPED;
    }

    if (g_audio_paused)
    {
        fb_sfxRuntimeUnlock();
        return FB_SFX_AUDIO_PAUSED;
    }

    status = FB_SFX_AUDIO_PLAYING;
    fb_sfxRuntimeUnlock();
    return status;
}


/* ------------------------------------------------------------------------- */
/* Status helpers                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioIsPlaying()

    Return non-zero if audio playback is currently active.
*/

int fb_sfxAudioIsPlaying(void)
{
    int playing;

    fb_sfxRuntimeLock();
    playing = (g_audio_playing && !g_audio_paused);
    fb_sfxRuntimeUnlock();

    return playing;
}


/*
    fb_sfxAudioIsPaused()

    Return non-zero if the audio stream is currently paused.
*/

int fb_sfxAudioIsPaused(void)
{
    int paused;

    fb_sfxRuntimeLock();
    paused = (g_audio_playing && g_audio_paused);
    fb_sfxRuntimeUnlock();

    return paused;
}


/* end of sfx_audio_status.c */
