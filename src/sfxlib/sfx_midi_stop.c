/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_stop.c

    Purpose:

        Implement the MIDI STOP command.

        This command stops playback of the currently playing
        MIDI file initiated by MIDI PLAY.

    Responsibilities:

        • stop MIDI playback
        • close the active MIDI file
        • reset MIDI playback state

    This file intentionally does NOT contain:

        • MIDI file parsing
        • MIDI synthesis
        • platform MIDI driver implementations

    Architectural overview:

        MIDI STOP
              │
        MIDI playback subsystem
              │
        platform MIDI driver
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------------- */
/* External MIDI playback state                                              */
/* ------------------------------------------------------------------------- */

/*
    These variables are owned by sfx_midi_play.c.
*/

extern int   g_midi_playing;


/* ------------------------------------------------------------------------- */
/* Internal helper                                                           */
/* ------------------------------------------------------------------------- */

int fb_sfxMidiStop(void)
{
    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();
    fb_sfxMidiStopInternal();
    fb_sfxRuntimeUnlock();
    fb_sfxMidiPauseReset();
    fb_sfxMidiJoinWorker();

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Status helper                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiStopped()

    Return non-zero if MIDI playback is stopped.
*/

int fb_sfxMidiStopped(void)
{
    int stopped;

    fb_sfxRuntimeLock();
    stopped = !g_midi_playing;
    fb_sfxRuntimeUnlock();

    return stopped;
}


/* end of sfx_midi_stop.c */
