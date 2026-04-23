/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_resume.c

    Purpose:

        Implement the MIDI RESUME command.

        This command resumes playback of a MIDI stream that
        was previously paused with MIDI PAUSE.

    Responsibilities:

        • resume MIDI playback
        • clear pause state safely
        • maintain consistent playback state

    This file intentionally does NOT contain:

        • MIDI file parsing
        • MIDI synthesis
        • platform MIDI driver implementations

    Architectural overview:

        MIDI RESUME
              │
        MIDI playback subsystem
              │
        event dispatch continues
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* External MIDI playback state                                              */
/* ------------------------------------------------------------------------- */

extern int g_midi_playing;
extern int g_midi_paused;


/* ------------------------------------------------------------------------- */
/* MIDI RESUME                                                               */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiResume()

    Resume playback of a paused MIDI stream.
*/

int fb_sfxMidiResume(void)
{
    fb_sfxRuntimeLock();
    if (!g_midi_playing)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    if (!g_midi_paused)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    g_midi_paused = 0;
    fb_sfxRuntimeUnlock();

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Status helpers                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiResumed()

    Return non-zero if MIDI playback is active and not paused.
*/

int fb_sfxMidiResumed(void)
{
    int resumed = 1;

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

    fb_sfxRuntimeUnlock();
    return resumed;
}


/* end of sfx_midi_resume.c */
