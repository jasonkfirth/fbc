/*
    Generic BSD-style MIDI backend using /dev/midi-style device nodes.
*/

#ifndef DISABLE_UNIX

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static int g_fb_sfx_unix_midi_fd = -1;

static int fb_sfxUnixMidiTryOpenPath(const char *path)
{
    if (!path)
        return -1;

    g_fb_sfx_unix_midi_fd = open(path, O_WRONLY | O_NONBLOCK);
    return (g_fb_sfx_unix_midi_fd >= 0) ? 0 : -1;
}

int fb_sfxMidiDriverOpen(int device)
{
    char path[64];

    if (g_fb_sfx_unix_midi_fd >= 0)
        fb_sfxMidiDriverClose();

    if (device <= 0)
    {
        if (fb_sfxUnixMidiTryOpenPath("/dev/midi") == 0)
            return 0;
        if (fb_sfxUnixMidiTryOpenPath("/dev/music") == 0)
            return 0;
        if (fb_sfxUnixMidiTryOpenPath("/dev/umidi0.0") == 0)
            return 0;
    }

    snprintf(path, sizeof(path), "/dev/umidi%d.0", (device < 0) ? 0 : device);
    return fb_sfxUnixMidiTryOpenPath(path);
}

void fb_sfxMidiDriverClose(void)
{
    if (g_fb_sfx_unix_midi_fd >= 0)
    {
        close(g_fb_sfx_unix_midi_fd);
        g_fb_sfx_unix_midi_fd = -1;
    }
}

int fb_sfxMidiDriverSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2)
{
    unsigned char message[3];
    size_t message_size = 3;
    ssize_t written;

    if (g_fb_sfx_unix_midi_fd < 0)
        return -1;

    message[0] = status;
    message[1] = data1;
    message[2] = data2;

    if ((status & 0xF0u) == 0xC0u || (status & 0xF0u) == 0xD0u)
        message_size = 2;

    written = write(g_fb_sfx_unix_midi_fd, message, message_size);
    return (written == (ssize_t)message_size) ? 0 : -1;
}

#endif
