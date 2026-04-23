/*
    macOS MIDI backend using CoreMIDI.
*/

#ifndef DISABLE_DARWIN

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#include <stdio.h>
#include <stdlib.h>

static MIDIClientRef g_fb_sfx_midi_client = 0;
static MIDIPortRef g_fb_sfx_midi_port = 0;
static MIDIEndpointRef g_fb_sfx_midi_destination = 0;

static int fb_sfxMidiDarwinDebugEnabled(void)
{
    const char *env = getenv("SFXLIB_DARWIN_DEBUG");
    return (env && *env && *env != '0');
}

#define MIDI_DBG(...) \
    do { if (fb_sfxMidiDarwinDebugEnabled()) fprintf(stderr, "SFX_MIDI_DARWIN: " __VA_ARGS__); } while (0)

int fb_sfxMidiDriverOpen(int device)
{
    ItemCount destination_count;
    OSStatus status;

    if (g_fb_sfx_midi_client)
        fb_sfxMidiDriverClose();

    status = MIDIClientCreate(CFSTR("sfxlib"), NULL, NULL, &g_fb_sfx_midi_client);
    if (status != noErr)
    {
        MIDI_DBG("MIDIClientCreate failed (status=%ld)\n", (long)status);
        return -1;
    }

    status = MIDIOutputPortCreate(g_fb_sfx_midi_client,
                                  CFSTR("sfxlib output"),
                                  &g_fb_sfx_midi_port);
    if (status != noErr)
    {
        MIDI_DBG("MIDIOutputPortCreate failed (status=%ld)\n", (long)status);
        fb_sfxMidiDriverClose();
        return -1;
    }

    destination_count = MIDIGetNumberOfDestinations();
    MIDI_DBG("CoreMIDI destinations=%lu\n", (unsigned long)destination_count);
    if (destination_count == 0)
    {
        fb_sfxMidiDriverClose();
        return -1;
    }

    if (device < 0 || device >= (int)destination_count)
        device = 0;

    g_fb_sfx_midi_destination = MIDIGetDestination((ItemCount)device);
    if (g_fb_sfx_midi_destination == 0)
    {
        MIDI_DBG("MIDIGetDestination(%d) returned null\n", device);
        fb_sfxMidiDriverClose();
        return -1;
    }

    MIDI_DBG("opened MIDI destination index=%d\n", device);

    return 0;
}

void fb_sfxMidiDriverClose(void)
{
    g_fb_sfx_midi_destination = 0;

    if (g_fb_sfx_midi_port)
    {
        MIDIPortDispose(g_fb_sfx_midi_port);
        g_fb_sfx_midi_port = 0;
    }

    if (g_fb_sfx_midi_client)
    {
        MIDIClientDispose(g_fb_sfx_midi_client);
        g_fb_sfx_midi_client = 0;
    }
}

int fb_sfxMidiDriverSend(unsigned char status,
                         unsigned char data1,
                         unsigned char data2)
{
    Byte buffer[64];
    MIDIPacketList *packet_list;
    MIDIPacket *packet;
    size_t message_size = 3;

    if (!g_fb_sfx_midi_port || !g_fb_sfx_midi_destination)
        return -1;

    if ((status & 0xF0u) == 0xC0u || (status & 0xF0u) == 0xD0u)
        message_size = 2;

    packet_list = (MIDIPacketList *)buffer;
    packet = MIDIPacketListInit(packet_list);

    if (message_size == 2)
    {
        unsigned char message[2];

        message[0] = status;
        message[1] = data1;
        packet = MIDIPacketListAdd(packet_list, sizeof(buffer), packet, 0, 2, message);
    }
    else
    {
        unsigned char message[3];

        message[0] = status;
        message[1] = data1;
        message[2] = data2;
        packet = MIDIPacketListAdd(packet_list, sizeof(buffer), packet, 0, 3, message);
    }

    if (!packet)
        return -1;

    if (MIDISend(g_fb_sfx_midi_port, g_fb_sfx_midi_destination, packet_list) != noErr)
    {
        MIDI_DBG("MIDISend failed for status=0x%02X data1=%u data2=%u\n",
                 (unsigned)status,
                 (unsigned)data1,
                 (unsigned)data2);
        return -1;
    }

    return 0;
}

#endif
