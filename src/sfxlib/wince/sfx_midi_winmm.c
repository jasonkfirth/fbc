/*
    FreeBASIC Sound Library support for Windows CE
    -----------------------------------------------

    File: sfx_midi_winmm.c

    Purpose:

        Mark native MIDI transport unavailable on the reference Windows CE
        SDK so the shared software FM synthesizer is selected.

    Responsibilities:

        - provide the platform MIDI transport symbols
        - fail native MIDI open and send requests predictably
        - route normal MIDI commands toward the software fallback

    This file intentionally does NOT contain:

        - calls to absent midiOut imports
        - MIDI file parsing
        - software synthesis, which is shared in sfx_midi_fm.c

    SDK constraint:

        CeGCC's COREDLL import library supplies waveOut PCM audio but does not
        expose midiOut.  Keeping that distinction here prevents packages from
        acquiring unresolved desktop multimedia imports.
*/

int fb_sfxMidiDriverOpen(int device)
{
    (void)device;
    return -1;
}

void fb_sfxMidiDriverClose(void)
{
}

int fb_sfxMidiDriverSend(unsigned char status,
    unsigned char data1, unsigned char data2)
{
    (void)status;
    (void)data1;
    (void)data2;
    return -1;
}

/* end of sfx_midi_winmm.c */
