/*
    DOS MPU-401 UART MIDI backend using the BLASTER P port when present.
*/

#ifndef DISABLE_MSDOS

#include "fb_sfx_msdos.h"

#include <string.h>

#ifdef __DJGPP__
#include <dos.h>
#endif

static FB_SFX_MSDOS_CONFIG g_fb_sfx_midi_msdos;
static int g_fb_sfx_midi_open = 0;

#ifdef __DJGPP__

static int fb_sfxMsdosMpuWaitWrite(int port)
{
    int timeout = 65535;

    while (timeout-- > 0)
    {
        if ((inportb(port + 1) & 0x40) == 0)
            return 1;
    }

    return 0;
}

static int fb_sfxMsdosMpuWrite(int port, unsigned char value)
{
    if (!fb_sfxMsdosMpuWaitWrite(port))
        return -1;

    outportb(port, value);
    return 0;
}

int fb_sfxMidiDriverOpen(int device)
{
    (void)device;

    if (fb_sfxMsdosParseBlaster(&g_fb_sfx_midi_msdos) != 0)
        return -1;

    if (g_fb_sfx_midi_msdos.mpu_port <= 0)
        return -1;

    if (fb_sfxMsdosMpuWrite(g_fb_sfx_midi_msdos.mpu_port, 0x3F) != 0)
        return -1;

    g_fb_sfx_midi_open = 1;
    return 0;
}

void fb_sfxMidiDriverClose(void)
{
    g_fb_sfx_midi_open = 0;
}

int fb_sfxMidiDriverSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2)
{
    if (!g_fb_sfx_midi_open)
        return -1;

    if (fb_sfxMsdosMpuWrite(g_fb_sfx_midi_msdos.mpu_port, status) != 0)
        return -1;
    if (fb_sfxMsdosMpuWrite(g_fb_sfx_midi_msdos.mpu_port, data1) != 0)
        return -1;

    if ((status & 0xF0u) != 0xC0u && (status & 0xF0u) != 0xD0u)
    {
        if (fb_sfxMsdosMpuWrite(g_fb_sfx_midi_msdos.mpu_port, data2) != 0)
            return -1;
    }

    return 0;
}

#else

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

#endif
