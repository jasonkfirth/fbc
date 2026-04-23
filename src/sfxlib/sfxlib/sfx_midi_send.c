/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_send.c

    Purpose:

        Implement the MIDI SEND command.

        This command sends a raw MIDI message directly to the
        currently opened MIDI output device.

        This allows BASIC programs to control external MIDI
        synthesizers, hardware devices, or the system MIDI
        synthesizer.

    Responsibilities:

        • validate MIDI message parameters
        • forward MIDI messages to the active platform driver
        • ensure a MIDI device is currently open

    This file intentionally does NOT contain:

        • MIDI device implementations
        • MIDI file parsing
        • synthesis logic

    Architectural overview:

        BASIC program
              │
        MIDI SEND
              │
        platform MIDI driver
              │
        synthesizer / MIDI hardware
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>


/* ------------------------------------------------------------------------- */
/* External MIDI state                                                       */
/* ------------------------------------------------------------------------- */

extern int fb_sfxMidiIsOpen(void);


/* ------------------------------------------------------------------------- */
/* Platform MIDI driver                                                      */
/* ------------------------------------------------------------------------- */

/*
    Platform drivers implement the actual MIDI message output.
*/

extern int fb_sfxMidiDriverSend(unsigned char status,
                                unsigned char data1,
                                unsigned char data2);


/* ------------------------------------------------------------------------- */
/* MIDI SEND                                                                 */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMidiSend()

    Send a raw MIDI message.

    status  - MIDI status byte
    data1   - first data byte
    data2   - second data byte
*/

int fb_sfxMidiSend(unsigned char status,
                   unsigned char data1,
                   unsigned char data2)
{
    int result;

    if (!fb_sfxMidiIsOpen())
    {
        SFX_DEBUG("sfx_midi_send: no MIDI device open");
        return -1;
    }

    /*
        Basic validation.

        MIDI data bytes must be within 0–127.
    */

    if (data1 > 127 || data2 > 127)
    {
        SFX_DEBUG("sfx_midi_send: invalid data byte");
        return -1;
    }

    fb_sfxRuntimeLock();
    result = fb_sfxMidiDriverSend(status, data1, data2);
    fb_sfxRuntimeUnlock();

    return result;
}


/* ------------------------------------------------------------------------- */
/* Convenience helpers                                                       */
/* ------------------------------------------------------------------------- */

/*
    Send NOTE ON event.
*/

int fb_sfxMidiNoteOn(unsigned char channel,
                     unsigned char note,
                     unsigned char velocity)
{
    unsigned char status = 0x90 | (channel & 0x0F);

    return fb_sfxMidiSend(status, note, velocity);
}


/*
    Send NOTE OFF event.
*/

int fb_sfxMidiNoteOff(unsigned char channel,
                      unsigned char note,
                      unsigned char velocity)
{
    unsigned char status = 0x80 | (channel & 0x0F);

    return fb_sfxMidiSend(status, note, velocity);
}


/*
    Send PROGRAM CHANGE event.
*/

int fb_sfxMidiProgramChange(unsigned char channel,
                            unsigned char program)
{
    unsigned char status = 0xC0 | (channel & 0x0F);
    int result;

    if (!fb_sfxMidiIsOpen())
        return -1;

    fb_sfxRuntimeLock();
    result = fb_sfxMidiDriverSend(status, program, 0);
    fb_sfxRuntimeUnlock();

    return result;
}


/* end of sfx_midi_send.c */
