/*
    Haiku MIDI backend using BMidiSynth for short-message playback.
*/

#ifndef DISABLE_HAIKU

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"

#include <MidiSynth.h>

static BMidiSynth *g_fb_sfx_haiku_synth = NULL;

extern "C" void fb_sfxMidiDriverClose(void);

extern "C" int fb_sfxMidiDriverOpen(int device)
{
    (void)device;

    if (g_fb_sfx_haiku_synth)
        fb_sfxMidiDriverClose();

    g_fb_sfx_haiku_synth = new BMidiSynth();
    if (!g_fb_sfx_haiku_synth)
        return -1;

    return 0;
}

extern "C" void fb_sfxMidiDriverClose(void)
{
    delete g_fb_sfx_haiku_synth;
    g_fb_sfx_haiku_synth = NULL;
}

extern "C" int fb_sfxMidiDriverSend(unsigned char status,
                                    unsigned char data1,
                                    unsigned char data2)
{
    int channel;

    if (!g_fb_sfx_haiku_synth)
        return -1;

    channel = (int)(status & 0x0Fu);

    switch (status & 0xF0u)
    {
        case 0x80u:
            g_fb_sfx_haiku_synth->NoteOff(channel, data1, data2);
            return 0;
        case 0x90u:
            if (data2 == 0)
                g_fb_sfx_haiku_synth->NoteOff(channel, data1, data2);
            else
                g_fb_sfx_haiku_synth->NoteOn(channel, data1, data2);
            return 0;
        case 0xB0u:
            g_fb_sfx_haiku_synth->ControlChange(channel, data1, data2);
            return 0;
        case 0xC0u:
            g_fb_sfx_haiku_synth->ProgramChange(channel, data1);
            return 0;
        default:
            return 0;
    }
}

#endif
