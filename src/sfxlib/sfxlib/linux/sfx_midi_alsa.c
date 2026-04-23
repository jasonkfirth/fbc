/*
    Linux MIDI backend using ALSA rawmidi output.
*/

#ifndef DISABLE_LINUX

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"

#include <alsa/asoundlib.h>
#include <stdio.h>

static snd_rawmidi_t *g_fb_sfx_linux_midi = NULL;
static int g_fb_sfx_linux_midi_open = 0;

static void fb_sfxLinuxMidiDeviceName(int device, char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0)
        return;

    if (device <= 0)
        snprintf(buffer, buffer_size, "default");
    else
        snprintf(buffer, buffer_size, "hw:%d,0,0", device);
}

int fb_sfxMidiDriverOpen(int device)
{
    char device_name[64];

    if (g_fb_sfx_linux_midi)
        fb_sfxMidiDriverClose();

    fb_sfxLinuxMidiDeviceName(device, device_name, sizeof(device_name));

    if (snd_rawmidi_open(NULL, &g_fb_sfx_linux_midi, device_name, 0) != 0)
        return -1;

    g_fb_sfx_linux_midi_open = 1;
    return 0;
}

void fb_sfxMidiDriverClose(void)
{
    if (g_fb_sfx_linux_midi)
    {
        snd_rawmidi_drain(g_fb_sfx_linux_midi);
        snd_rawmidi_close(g_fb_sfx_linux_midi);
        g_fb_sfx_linux_midi = NULL;
    }

    g_fb_sfx_linux_midi_open = 0;
}

int fb_sfxMidiDriverSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2)
{
    unsigned char message[3];
    int message_size = 3;
    int written;

    if (!g_fb_sfx_linux_midi || !g_fb_sfx_linux_midi_open)
        return -1;

    message[0] = status;
    message[1] = data1;
    message[2] = data2;

    if ((status & 0xF0u) == 0xC0u || (status & 0xF0u) == 0xD0u)
        message_size = 2;

    written = (int)snd_rawmidi_write(g_fb_sfx_linux_midi, message, (size_t)message_size);
    if (written != message_size)
        return -1;

    snd_rawmidi_drain(g_fb_sfx_linux_midi);
    return 0;
}

#endif
