/*
    FreeBASIC Sound Library support for AmigaOS
    ----------------------------------------

    File: sfx_driver_ahi.c

    Purpose:

        Stream sfxlib output through AMIGA ahi.device.

    Responsibilities:

        - open the user's configured AHI output unit
        - convert interleaved float samples to native signed 16-bit PCM
        - submit complete blocking writes to the AHI device
        - coordinate the background mixer worker with device lifecycle

    This file intentionally does NOT contain:

        - low-level AHI hardware allocation
        - interactive audio-mode requesters
        - mixer or synthesizer policy

    Portability:

        AHIST_M16S and AHIST_S16S describe native-endian WORD samples.  The
        same implementation therefore serves AMIGA m68k, ARM, and x86_64
        without embedding an architecture baseline in the sound backend.
*/

#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_amiga.h"

#include <devices/ahi.h>
#include <exec/io.h>
#include <proto/exec.h>

#include <limits.h>
#include <stdlib.h>

static struct MsgPort *g_amiga_ahi_port;
static struct AHIRequest *g_amiga_ahi_request;
static short *g_amiga_ahi_pcm;
static int g_amiga_ahi_pcm_capacity;
static int g_amiga_ahi_channels = FB_SFX_DEFAULT_CHANNELS;
static int g_amiga_ahi_rate = FB_SFX_DEFAULT_RATE;
static int g_amiga_ahi_open;

static int amiga_ahiEnsurePcmCapacity(int samples)
{
    short *next_buffer;
    int next_capacity;

    if (samples <= g_amiga_ahi_pcm_capacity)
        return 0;
    if (samples <= 0 || samples > INT_MAX / (int)sizeof(short))
        return -1;

    next_capacity = (g_amiga_ahi_pcm_capacity > 0)
        ? g_amiga_ahi_pcm_capacity
        : 1024;
    while (next_capacity < samples)
    {
        if (next_capacity > INT_MAX / 2)
            return -1;
        next_capacity *= 2;
    }

    next_buffer = (short *)realloc(g_amiga_ahi_pcm,
        (size_t)next_capacity * sizeof(short));
    if (next_buffer == NULL)
        return -1;

    g_amiga_ahi_pcm = next_buffer;
    g_amiga_ahi_pcm_capacity = next_capacity;
    return 0;
}

static void amiga_ahiExit(void)
{
    fb_sfxAmigaWorkerStop();

    if (g_amiga_ahi_open)
    {
        CloseDevice((struct IORequest *)g_amiga_ahi_request);
        g_amiga_ahi_open = FALSE;
    }

    if (g_amiga_ahi_request != NULL)
    {
        DeleteIORequest((struct IORequest *)g_amiga_ahi_request);
        g_amiga_ahi_request = NULL;
    }

    if (g_amiga_ahi_port != NULL)
    {
        DeleteMsgPort(g_amiga_ahi_port);
        g_amiga_ahi_port = NULL;
    }

    free(g_amiga_ahi_pcm);
    g_amiga_ahi_pcm = NULL;
    g_amiga_ahi_pcm_capacity = 0;
}

static int amiga_ahiInit(int rate, int channels, int buffer_frames, int flags)
{
    (void)flags;

    if (g_amiga_ahi_open)
        return 0;
    if (rate <= 0)
        rate = FB_SFX_DEFAULT_RATE;
    if (channels <= 0)
        channels = FB_SFX_DEFAULT_CHANNELS;
    if (channels != 1 && channels != 2)
        return -1;

    g_amiga_ahi_port = CreateMsgPort();
    if (g_amiga_ahi_port == NULL)
        goto fail;

    g_amiga_ahi_request = (struct AHIRequest *)CreateIORequest(
        g_amiga_ahi_port, sizeof(struct AHIRequest));
    if (g_amiga_ahi_request == NULL)
        goto fail;

    g_amiga_ahi_request->ahir_Version = 4;
    if (OpenDevice((CONST_STRPTR)AHINAME, AHI_DEFAULT_UNIT,
        (struct IORequest *)g_amiga_ahi_request, 0) != 0)
    {
        goto fail;
    }

    g_amiga_ahi_open = TRUE;
    g_amiga_ahi_rate = rate;
    g_amiga_ahi_channels = channels;

    if (fb_sfxAmigaWorkerStart(buffer_frames) != 0)
        goto fail;

    SFX_DEBUG("amiga_ahi: initialized rate=%d channels=%d",
        g_amiga_ahi_rate, g_amiga_ahi_channels);
    return 0;

fail:
    amiga_ahiExit();
    return -1;
}

static int amiga_ahiWrite(const float *samples, int frames)
{
    int frame_bytes;
    int sample_count;
    struct MsgPort *reply_port;
    LONG result;

    if (!g_amiga_ahi_open || samples == NULL || frames <= 0)
        return -1;
    if (frames > INT_MAX / g_amiga_ahi_channels)
        return -1;

    sample_count = frames * g_amiga_ahi_channels;
    if (amiga_ahiEnsurePcmCapacity(sample_count) != 0)
        return -1;

    fb_sfxDriverDiagnostics("AMIGA AHI", samples, frames,
        g_amiga_ahi_channels);
    fb_sfxConvertFloatToS16(samples, g_amiga_ahi_pcm, sample_count);

    frame_bytes = g_amiga_ahi_channels * (int)sizeof(short);
    g_amiga_ahi_request->ahir_Std.io_Command = CMD_WRITE;
    g_amiga_ahi_request->ahir_Std.io_Data = g_amiga_ahi_pcm;
    g_amiga_ahi_request->ahir_Std.io_Length =
        (ULONG)(sample_count * (int)sizeof(short));
    g_amiga_ahi_request->ahir_Std.io_Offset = 0;
    g_amiga_ahi_request->ahir_Type = (g_amiga_ahi_channels == 2)
        ? AHIST_S16S
        : AHIST_M16S;
    g_amiga_ahi_request->ahir_Frequency = (ULONG)g_amiga_ahi_rate;
    g_amiga_ahi_request->ahir_Volume = 0x10000;
    g_amiga_ahi_request->ahir_Position = 0x8000;
    g_amiga_ahi_request->ahir_Link = NULL;

    /* AHI writes can originate on the feeder or foreground task. A message
       port's signal belongs to its creating task, so each synchronous write
       uses a caller-owned reply port while the shared I/O lock excludes other
       writers. Reusing the initialization task's port would strand WaitIO. */
    reply_port = CreateMsgPort();
    if (reply_port == NULL)
        return -1;
    g_amiga_ahi_request->ahir_Std.io_Message.mn_ReplyPort = reply_port;
    result = DoIO((struct IORequest *)g_amiga_ahi_request);
    g_amiga_ahi_request->ahir_Std.io_Message.mn_ReplyPort = g_amiga_ahi_port;
    DeleteMsgPort(reply_port);
    if (result != 0)
        return -1;

    return (int)(g_amiga_ahi_request->ahir_Std.io_Actual /
        (ULONG)frame_bytes);
}

static int amiga_ahiDeviceList(void)
{
    return 1;
}

const FB_SFX_DRIVER fb_sfxDriverAmigaAhi =
{
    "AMIGA AHI",
    FB_SFX_DRIVER_CAP_BACKGROUND | FB_SFX_DRIVER_CAP_BLOCKING,
    amiga_ahiInit,
    amiga_ahiExit,
    amiga_ahiWrite,
    NULL,
    NULL,
    amiga_ahiDeviceList,
    NULL
};

/* end of sfx_driver_ahi.c */
