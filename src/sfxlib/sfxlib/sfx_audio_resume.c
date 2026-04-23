/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_audio_resume.c

    Purpose:

        Implement the AUDIO RESUME command.

        This command resumes playback of an audio stream that was
        previously paused with AUDIO PAUSE.

    Responsibilities:

        • resume playback of a paused audio stream
        • restore mixer input flow for the active stream
        • update runtime playback state safely

    This file intentionally does NOT contain:

        • audio decoding logic
        • mixer algorithms
        • driver playback logic

    Architectural overview:

        AUDIO RESUME
              │
        streaming subsystem
              │
        mixer input restored
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* External stream state                                                     */
/* ------------------------------------------------------------------------- */

/*
    The stream state is maintained in the audio streaming subsystem
    implemented by sfx_audio_play.c and related modules.
*/

extern int g_audio_playing;
extern int g_audio_paused;


/* ------------------------------------------------------------------------- */
/* AUDIO RESUME                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioResume()

    Resume playback of the currently paused audio stream.
*/

void fb_sfxAudioResume(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();
    if (!g_audio_playing)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    if (!g_audio_paused)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    g_audio_paused = 0;
    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Resume status                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioResumed()

    Returns non-zero if playback is active and not paused.
*/

int fb_sfxAudioResumed(void)
{
    int resumed;

    fb_sfxRuntimeLock();
    if (!g_audio_playing)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    if (g_audio_paused)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    resumed = 1;
    fb_sfxRuntimeUnlock();

    return resumed;
}


/* end of sfx_audio_resume.c */
