/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_open.c

    Purpose:

        Implement the MIDI OPEN command.

        This command opens a MIDI output device that can be used
        to send MIDI events or play MIDI files through the system
        synthesizer.

        If the native MIDI transport cannot be opened, the command routes
        events to sfxlib's small C software FM synthesizer.

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
#include <string.h>


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
static int g_midi_software = 0;


/* ------------------------------------------------------------------------- */
/* MIDI output routing                                                       */
/* ------------------------------------------------------------------------- */

/*
    SFXLIB_MIDI_DRIVER=fm provides a deterministic way to select the fallback
    on a machine that also has a native MIDI backend. This is useful for
    testing and for programs that need the same small synthesizer everywhere.
*/

static int fb_sfxMidiSoftwareRequested(void)
{
    const char *driver;

    driver = getenv("SFXLIB_MIDI_DRIVER");
    if (!driver)
        return 0;

    return (strcmp(driver, "fm") == 0 ||
            strcmp(driver, "software") == 0);
}

void fb_sfxMidiOutputClose(void)
{
    if (g_midi_software)
        fb_sfxMidiSoftwareClose();
    else
        fb_sfxMidiDriverClose();

    g_midi_software = 0;
}

int fb_sfxMidiOutputSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2)
{
    if (g_midi_software)
        return fb_sfxMidiSoftwareSend(status, data1, data2);

    return fb_sfxMidiDriverSend(status, data1, data2);
}

void fb_sfxMidiOutputSilence(void)
{
    if (g_midi_software)
        fb_sfxMidiSoftwareSilence();
}

void fb_sfxMidiOutputReleaseAll(void)
{
    if (g_midi_software)
        fb_sfxMidiSoftwareReleaseAll();
}

void fb_sfxMidiOutputPause(int paused)
{
    if (g_midi_software)
        fb_sfxMidiSoftwarePause(paused);
}


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
    int software_requested;

    if (!fb_sfxEnsureInitialized())
        return -1;

    if (device < 0)
        return -1;

    /*
        A backend switch cannot preserve a worker that is still dispatching
        the previous file. Stop it before replacing the output route.
    */

    fb_sfxMidiStop();

    fb_sfxRuntimeLock();
    if (g_midi_open)
    {
        fb_sfxMidiOutputClose();
        g_midi_device = -1;
        g_midi_open = 0;
    }

    software_requested = fb_sfxMidiSoftwareRequested();
    result = -1;

    if (!software_requested)
        result = fb_sfxMidiDriverOpen(device);

    if (result != 0)
    {
        result = fb_sfxMidiSoftwareOpen();
        if (result == 0)
        {
            g_midi_software = 1;
            SFX_DEBUG("sfx_midi_open: using software FM fallback");
        }
    }

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
