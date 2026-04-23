/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_audio_pause.c

    Purpose:

        Implement the AUDIO PAUSE command.

        This command temporarily suspends playback of the currently
        active audio stream started by AUDIO PLAY or AUDIO LOOP.

        Unlike AUDIO STOP, pausing preserves the current playback
        position so playback may resume later.

    Responsibilities:

        • pause the active audio stream
        • preserve the file position
        • update streaming state safely

    This file intentionally does NOT contain:

        • audio decoding logic
        • mixer algorithms
        • platform driver interaction

    Architectural overview:

        AUDIO PAUSE
              │
        streaming subsystem
              │
        mixer input suspension
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>


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
/* AUDIO PAUSE                                                               */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioPause()

    Pause the currently active audio stream.
*/

void fb_sfxAudioPause(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();
    if (!g_audio_playing)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    if (g_audio_paused)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    g_audio_paused = 1;
    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Internal helper                                                           */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAudioPaused()

    Return non-zero if the current audio stream is paused.
*/

int fb_sfxAudioPaused(void)
{
    int paused;

    fb_sfxRuntimeLock();
    paused = g_audio_paused;
    fb_sfxRuntimeUnlock();

    return paused;
}


/* end of sfx_audio_pause.c */
