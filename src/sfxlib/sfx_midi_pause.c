/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_pause.c

    Purpose:

        Implement the MIDI PAUSE command.

        This command pauses playback of the currently active
        MIDI stream started by MIDI PLAY.

        Unlike MIDI STOP, pausing preserves the playback
        position so that playback may resume later.

    Responsibilities:

        • pause MIDI playback
        • preserve playback position
        • update MIDI playback state

    This file intentionally does NOT contain:

        • MIDI file parsing
        • MIDI synthesis
        • platform MIDI driver implementations

    Architectural overview:

        MIDI PAUSE
              │
        MIDI playback subsystem
              │
        event dispatch suspended
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* External MIDI playback state                                              */
/* ------------------------------------------------------------------------- */

/*
    These variables are owned by sfx_midi_play.c.
*/

extern int g_midi_playing;


/* ------------------------------------------------------------------------- */
/* Pause state                                                               */
/* ------------------------------------------------------------------------- */

int g_midi_paused = 0;


/* ------------------------------------------------------------------------- */
/* MIDI PAUSE                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiPause()

    Pause MIDI playback.
*/

int fb_sfxMidiPause(void)
{
    fb_sfxRuntimeLock();
    if (!g_midi_playing)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    if (g_midi_paused)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    g_midi_paused = 1;
    fb_sfxRuntimeUnlock();

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Status helpers                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiPaused()

    Return non-zero if MIDI playback is currently paused.
*/

int fb_sfxMidiPaused(void)
{
    int paused;

    fb_sfxRuntimeLock();
    paused = g_midi_paused;
    fb_sfxRuntimeUnlock();

    return paused;
}


/*
    fb_sfxMidiPauseReset()

    Reset pause state (used internally when playback stops).
*/

void fb_sfxMidiPauseReset(void)
{
    fb_sfxRuntimeLock();
    g_midi_paused = 0;
    fb_sfxRuntimeUnlock();
}


/* end of sfx_midi_pause.c */
