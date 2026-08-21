/*
    FreeBASIC Sound Library support for AROS
    ----------------------------------------

    File: sfx_driver_ahi.c

    Purpose:

        Stream sfxlib output through AROS ahi.device.

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
        same implementation therefore serves AROS m68k, ARM, and x86_64
        without embedding an architecture baseline in the sound backend.
*/

#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_aros.h"

#include <devices/ahi.h>
#include <exec/io.h>
#include <proto/exec.h>

#include <limits.h>
#include <stdlib.h>

static struct MsgPort *g_aros_ahi_port;
static struct AHIRequest *g_aros_ahi_request;
static short *g_aros_ahi_pcm;
static int g_aros_ahi_pcm_capacity;
static int g_aros_ahi_channels = FB_SFX_DEFAULT_CHANNELS;
static int g_aros_ahi_rate = FB_SFX_DEFAULT_RATE;
static int g_aros_ahi_open;

static int aros_ahiEnsurePcmCapacity(int samples)
{
    short *next_buffer;
    int next_capacity;

    if (samples <= g_aros_ahi_pcm_capacity)
        return 0;
    if (samples <= 0 || samples > INT_MAX / (int)sizeof(short))
        return -1;

    next_capacity = (g_aros_ahi_pcm_capacity > 0)
        ? g_aros_ahi_pcm_capacity
        : 1024;
    while (next_capacity < samples)
    {
        if (next_capacity > INT_MAX / 2)
            return -1;
        next_capacity *= 2;
    }

    next_buffer = (short *)realloc(g_aros_ahi_pcm,
        (size_t)next_capacity * sizeof(short));
    if (next_buffer == NULL)
        return -1;

    g_aros_ahi_pcm = next_buffer;
    g_aros_ahi_pcm_capacity = next_capacity;
    return 0;
}

static void aros_ahiExit(void)
{
    fb_sfxArosWorkerStop();

    if (g_aros_ahi_open)
    {
        CloseDevice((struct IORequest *)g_aros_ahi_request);
        g_aros_ahi_open = FALSE;
    }

    if (g_aros_ahi_request != NULL)
    {
        DeleteIORequest((struct IORequest *)g_aros_ahi_request);
        g_aros_ahi_request = NULL;
    }

    if (g_aros_ahi_port != NULL)
    {
        DeleteMsgPort(g_aros_ahi_port);
        g_aros_ahi_port = NULL;
    }

    free(g_aros_ahi_pcm);
    g_aros_ahi_pcm = NULL;
    g_aros_ahi_pcm_capacity = 0;
}

static int aros_ahiInit(int rate, int channels, int buffer_frames, int flags)
{
    (void)flags;

    if (g_aros_ahi_open)
        return 0;
    if (rate <= 0)
        rate = FB_SFX_DEFAULT_RATE;
    if (channels <= 0)
        channels = FB_SFX_DEFAULT_CHANNELS;
    if (channels != 1 && channels != 2)
        return -1;

    g_aros_ahi_port = CreateMsgPort();
    if (g_aros_ahi_port == NULL)
        goto fail;

    g_aros_ahi_request = (struct AHIRequest *)CreateIORequest(
        g_aros_ahi_port, sizeof(struct AHIRequest));
    if (g_aros_ahi_request == NULL)
        goto fail;

    g_aros_ahi_request->ahir_Version = 4;
    if (OpenDevice((CONST_STRPTR)AHINAME, AHI_DEFAULT_UNIT,
        (struct IORequest *)g_aros_ahi_request, 0) != 0)
    {
        goto fail;
    }

    g_aros_ahi_open = TRUE;
    g_aros_ahi_rate = rate;
    g_aros_ahi_channels = channels;

    if (fb_sfxArosWorkerStart(buffer_frames) != 0)
        goto fail;

    SFX_DEBUG("aros_ahi: initialized rate=%d channels=%d",
        g_aros_ahi_rate, g_aros_ahi_channels);
    return 0;

fail:
    aros_ahiExit();
    return -1;
}

static int aros_ahiWrite(const float *samples, int frames)
{
    int frame_bytes;
    int sample_count;

    if (!g_aros_ahi_open || samples == NULL || frames <= 0)
        return -1;
    if (frames > INT_MAX / g_aros_ahi_channels)
        return -1;

    sample_count = frames * g_aros_ahi_channels;
    if (aros_ahiEnsurePcmCapacity(sample_count) != 0)
        return -1;

    fb_sfxDriverDiagnostics("AROS AHI", samples, frames,
        g_aros_ahi_channels);
    fb_sfxConvertFloatToS16(samples, g_aros_ahi_pcm, sample_count);

    frame_bytes = g_aros_ahi_channels * (int)sizeof(short);
    g_aros_ahi_request->ahir_Std.io_Command = CMD_WRITE;
    g_aros_ahi_request->ahir_Std.io_Data = g_aros_ahi_pcm;
    g_aros_ahi_request->ahir_Std.io_Length =
        (ULONG)(sample_count * (int)sizeof(short));
    g_aros_ahi_request->ahir_Std.io_Offset = 0;
    g_aros_ahi_request->ahir_Type = (g_aros_ahi_channels == 2)
        ? AHIST_S16S
        : AHIST_M16S;
    g_aros_ahi_request->ahir_Frequency = (ULONG)g_aros_ahi_rate;
    g_aros_ahi_request->ahir_Volume = 0x10000;
    g_aros_ahi_request->ahir_Position = 0x8000;
    g_aros_ahi_request->ahir_Link = NULL;

    if (DoIO((struct IORequest *)g_aros_ahi_request) != 0)
        return -1;

    return (int)(g_aros_ahi_request->ahir_Std.io_Actual /
        (ULONG)frame_bytes);
}

static int aros_ahiDeviceList(void)
{
    return 1;
}

const FB_SFX_DRIVER fb_sfxDriverArosAhi =
{
    "AROS AHI",
    FB_SFX_DRIVER_CAP_BACKGROUND | FB_SFX_DRIVER_CAP_BLOCKING,
    aros_ahiInit,
    aros_ahiExit,
    aros_ahiWrite,
    NULL,
    NULL,
    aros_ahiDeviceList,
    NULL
};

/* end of sfx_driver_ahi.c */
