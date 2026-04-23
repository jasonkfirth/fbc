/*
    Windows MIDI backend using WinMM midiOut.
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"

#include <windows.h>
#include <mmsystem.h>

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

static HMIDIOUT g_fb_sfx_midi_out = NULL;
static UINT g_fb_sfx_midi_device = MIDI_MAPPER;

void fb_sfxMidiDriverClose(void);

int fb_sfxMidiDriverOpen(int device)
{
    MMRESULT result;

    if (g_fb_sfx_midi_out)
        fb_sfxMidiDriverClose();

    if (device < 0)
        return -1;

    g_fb_sfx_midi_device = (UINT)device;
    result = midiOutOpen(&g_fb_sfx_midi_out,
                         g_fb_sfx_midi_device,
                         0,
                         0,
                         CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR)
    {
        g_fb_sfx_midi_out = NULL;
        return -1;
    }

    return 0;
}

void fb_sfxMidiDriverClose(void)
{
    if (!g_fb_sfx_midi_out)
        return;

    midiOutReset(g_fb_sfx_midi_out);
    midiOutClose(g_fb_sfx_midi_out);
    g_fb_sfx_midi_out = NULL;
}

int fb_sfxMidiDriverSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2)
{
    DWORD message;

    if (!g_fb_sfx_midi_out)
        return -1;

    message = (DWORD)status |
              ((DWORD)data1 << 8) |
              ((DWORD)data2 << 16);

    return (midiOutShortMsg(g_fb_sfx_midi_out, message) == MMSYSERR_NOERROR)
        ? 0
        : -1;
}
