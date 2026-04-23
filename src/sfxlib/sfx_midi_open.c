/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_open.c

    Purpose:

        Implement the MIDI OPEN command.

        This command opens a MIDI output device that can be used
        to send MIDI events or play MIDI files through the system
        synthesizer.

    Responsibilities:

        • open a MIDI output device
        • track the active MIDI device
        • provide a stable interface for other MIDI commands

    This file intentionally does NOT contain:

        • MIDI file playback logic
        • MIDI message formatting
        • platform-specific implementations

    Architectural overview:

        MIDI OPEN
              │
        platform MIDI driver
              │
        system synthesizer
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>


/* ------------------------------------------------------------------------- */
/* MIDI device state                                                         */
/* ------------------------------------------------------------------------- */

/*
    The active MIDI device identifier.

    For cross-platform consistency we store the device index
    and allow platform backends to interpret it.
*/

int g_midi_device = -1;
int g_midi_open   = 0;


/* ------------------------------------------------------------------------- */
/* Platform entry points                                                     */
/* ------------------------------------------------------------------------- */

/*
    Platform backends implement the actual MIDI device opening.
*/

extern int fb_sfxMidiDriverOpen(int device);


/* ------------------------------------------------------------------------- */
/* MIDI OPEN                                                                 */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiOpen()

    Open a MIDI device by index.
*/

int fb_sfxMidiOpen(int device)
{
    int result;

    if (!fb_sfxEnsureInitialized())
        return -1;

    fb_sfxRuntimeLock();
    if (device < 0)
    {
        fb_sfxRuntimeUnlock();
        return -1;
    }

    result = fb_sfxMidiDriverOpen(device);

    if (result != 0)
    {
        fb_sfxRuntimeUnlock();
        SFX_DEBUG("sfx_midi_open: failed to open device %d", device);
        return -1;
    }

    g_midi_device = device;
    g_midi_open   = 1;

    fb_sfxRuntimeUnlock();
    return 0;
}


/* ------------------------------------------------------------------------- */
/* Status helpers                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiIsOpen()

    Return non-zero if a MIDI device is currently open.
*/

int fb_sfxMidiIsOpen(void)
{
    int is_open;

    fb_sfxRuntimeLock();
    is_open = g_midi_open;
    fb_sfxRuntimeUnlock();

    return is_open;
}


/*
    fb_sfxMidiDevice()

    Return the current MIDI device index.
*/

int fb_sfxMidiDevice(void)
{
    int device;

    fb_sfxRuntimeLock();
    device = g_midi_device;
    fb_sfxRuntimeUnlock();

    return device;
}


/* end of sfx_midi_open.c */
