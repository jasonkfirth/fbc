/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_close.c

    Purpose:

        Implement the MIDI CLOSE command.

        This command closes the currently active MIDI output
        device previously opened with MIDI OPEN.

    Responsibilities:

        • close the active MIDI device
        • reset MIDI subsystem state
        • safely coordinate with the platform MIDI backend

    This file intentionally does NOT contain:

        • MIDI playback logic
        • MIDI message generation
        • platform-specific implementations

    Architectural overview:

        MIDI CLOSE
              │
        platform MIDI driver
              │
        system synthesizer
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* MIDI CLOSE                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiClose()

    Close the currently active MIDI device.
*/

int fb_sfxMidiClose(void)
{
    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxMidiStop();

    fb_sfxRuntimeLock();
    if (!g_midi_open)
    {
        fb_sfxRuntimeUnlock();
        return 0;
    }

    /*
        Route shutdown through the same layer used by MIDI SEND. A
        software-backed open has no platform MIDI handle to close.
    */
    fb_sfxMidiOutputSilence();
    fb_sfxMidiOutputPause(0);
    fb_sfxMidiOutputClose();

    g_midi_device = -1;
    g_midi_open   = 0;

    fb_sfxRuntimeUnlock();
    return 0;
}


/* ------------------------------------------------------------------------- */
/* MIDI state helper                                                         */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiClosed()

    Return non-zero if the MIDI device is currently closed.
*/

int fb_sfxMidiClosed(void)
{
    int closed;

    fb_sfxRuntimeLock();
    closed = !g_midi_open;
    fb_sfxRuntimeUnlock();

    return closed;
}


/* end of sfx_midi_close.c */
