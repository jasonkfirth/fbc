/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_driver_stub.c

    Purpose:

        Provide the fallback MIDI driver for targets that do not
        have a platform MIDI output backend.

    Responsibilities:

        • satisfy the internal MIDI driver symbols on unsupported targets
        • report unsupported MIDI output honestly

    This file intentionally does NOT contain:

        • a silent no-op MIDI backend
        • platform MIDI device discovery
        • MIDI file parsing

    Backend contract:

        MIDI OPEN must fail when no real output path exists.  Returning
        success here would make MIDI commands look usable while dropping
        every message.
*/

#if !defined(_WIN32) && \
    !defined(__CYGWIN__) && \
    !defined(__DJGPP__) && \
    !defined(__linux__) && \
    !defined(__APPLE__) && \
    !defined(__FreeBSD__) && \
    !defined(__NetBSD__) && \
    !defined(__OpenBSD__) && \
    !defined(__DragonFly__) && \
    !defined(__sun) && \
    !defined(__HAIKU__) && \
    !defined(HOST_XBOX)

int fb_sfxMidiDriverOpen(int device)
{
    (void)device;
    return -1;
}

void fb_sfxMidiDriverClose(void)
{
}

int fb_sfxMidiDriverSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2)
{
    (void)status;
    (void)data1;
    (void)data2;
    return -1;
}

#endif

/* end of sfx_midi_driver_stub.c */
