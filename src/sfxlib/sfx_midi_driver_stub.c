/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_driver_stub.c

    Purpose:

        Mark the platform MIDI transport as unavailable on targets that do
        not have a native MIDI output backend.

    Responsibilities:

        - satisfy the internal MIDI driver symbols on unsupported targets
        - let the generic MIDI router select the software FM fallback

    This file intentionally does NOT contain:

        - a silent no-op MIDI backend
        - software synthesis, which is implemented in sfx_midi_fm.c
        - platform MIDI device discovery
        - MIDI file parsing

    Backend contract:

        This transport must fail MIDI OPEN. The command-level router then
        tries the C software synthesizer, whose waveform follows the normal
        sfxlib mixer and audio-driver path.
*/

#if !defined(_WIN32) && \
    !defined(__CYGWIN__) && \
    !defined(__DJGPP__) && \
    (!defined(__linux__) || defined(__ANDROID__)) && \
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

#else

/*
    The build graph includes this portable selector on every target.  Keep a
    declaration in native-backend builds so the translation unit remains valid
    ISO C after the stub implementation is compiled out.
*/
typedef int FB_SFX_MIDI_NATIVE_DRIVER_PRESENT;

#endif

/* end of sfx_midi_driver_stub.c */
