/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_capture_wasapi.c

    Purpose:

        Implement audio capture using the Windows WASAPI subsystem.

        This module provides microphone / line-in capture support
        for sfxlib. Captured audio can be used for:

            • recording
            • audio visualization
            • mixer loopback analysis
            • diagnostic testing

    Responsibilities:

        • initialize WASAPI capture devices
        • read captured audio frames
        • feed captured samples into the capture subsystem
        • safely shut down capture devices

    This file intentionally does NOT contain:

        • file recording logic
        • mixer processing
        • playback functionality
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "fb_sfx_win32.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Capture state                                                             */
/* ------------------------------------------------------------------------- */

static IMMDeviceEnumerator *g_cap_enum = NULL;
static IMMDevice *g_cap_device = NULL;
static IAudioClient *g_cap_audio = NULL;
static IAudioCaptureClient *g_cap_client = NULL;
static WAVEFORMATEX *g_cap_format = NULL;
static int g_capture_active = 0;
static int g_capture_com_ready = 0;
static const GUID sfx_capture_float_guid =
{
    0x00000003, 0x0000, 0x0010,
    { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 }
};


/* ------------------------------------------------------------------------- */
/* Debug helper                                                              */
/* ------------------------------------------------------------------------- */

static int wasapi_capture_debug(void)
{
    const char *e = getenv("SFXLIB_DEBUG");
    return (e && *e && *e != '0');
}

#define CAP_DBG(...) \
    do { if (wasapi_capture_debug()) fprintf(stderr,"WASAPI CAPTURE: " __VA_ARGS__); } while(0)

static void capture_release_interfaces(void)
{
    if (g_cap_client)
    {
        g_cap_client->lpVtbl->Release(g_cap_client);
        g_cap_client = NULL;
    }

    if (g_cap_audio)
    {
        g_cap_audio->lpVtbl->Release(g_cap_audio);
        g_cap_audio = NULL;
    }

    if (g_cap_device)
    {
        g_cap_device->lpVtbl->Release(g_cap_device);
        g_cap_device = NULL;
    }

    if (g_cap_enum)
    {
        g_cap_enum->lpVtbl->Release(g_cap_enum);
        g_cap_enum = NULL;
    }

    if (g_cap_format)
    {
        CoTaskMemFree(g_cap_format);
        g_cap_format = NULL;
    }
}

static int capture_format_is_float(const WAVEFORMATEX *fmt)
{
    if (!fmt)
        return 0;

    if (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        return 1;

    if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        const WAVEFORMATEXTENSIBLE *ext = (const WAVEFORMATEXTENSIBLE *)fmt;
        return IsEqualGUID(&ext->SubFormat, &sfx_capture_float_guid);
    }

    return 0;
}

static int capture_bytes_per_sample(const WAVEFORMATEX *fmt)
{
    if (!fmt || fmt->nChannels == 0)
        return 0;

    return (int)(fmt->nBlockAlign / fmt->nChannels);
}

static void capture_convert_to_float(const BYTE *src,
                                     float *dst,
                                     int frames,
                                     const WAVEFORMATEX *fmt,
                                     int out_channels)
{
    int frame;
    int in_channels;
    int bytes_per_sample;
    int use_float;

    if (!src || !dst || !fmt || frames <= 0)
        return;

    in_channels = fmt->nChannels > 0 ? (int)fmt->nChannels : 1;
    bytes_per_sample = capture_bytes_per_sample(fmt);
    use_float = capture_format_is_float(fmt);

    for (frame = 0; frame < frames; ++frame)
    {
        float left = 0.0f;
        float right = 0.0f;
        int ch;

        for (ch = 0; ch < in_channels; ++ch)
        {
            const BYTE *sample_ptr = src + (((frame * in_channels) + ch) * bytes_per_sample);
            float sample = 0.0f;

            if (use_float && bytes_per_sample == (int)sizeof(float))
            {
                sample = ((const float *)src)[(frame * in_channels) + ch];
            }
            else if (!use_float && bytes_per_sample == 2)
            {
                short pcm;
                memcpy(&pcm, sample_ptr, sizeof(pcm));
                sample = (float)pcm / 32768.0f;
            }
            else if (!use_float && bytes_per_sample == 4)
            {
                int pcm;
                memcpy(&pcm, sample_ptr, sizeof(pcm));
                sample = (float)pcm / 2147483648.0f;
            }

            if (ch == 0)
                left = sample;
            else if (ch == 1)
                right = sample;
        }

        if (in_channels == 1)
            right = left;

        if (out_channels <= 1)
        {
            dst[frame] = (left + right) * 0.5f;
        }
        else
        {
            dst[frame * out_channels] = left;
            dst[(frame * out_channels) + 1] = right;
        }
    }
}


/* ------------------------------------------------------------------------- */
/* Capture initialization                                                    */
/* ------------------------------------------------------------------------- */

int fb_sfxPlatformCaptureStart(void)
{
    HRESULT hr;
    REFERENCE_TIME buffer_duration = 1000000;

    capture_release_interfaces();
    g_capture_active = 0;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return -1;

    if (SUCCEEDED(hr))
        g_capture_com_ready = 1;

    hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        &IID_IMMDeviceEnumerator,
        (void**)&g_cap_enum
    );

    if (FAILED(hr))
    {
        capture_release_interfaces();
        return -1;
    }

    hr = g_cap_enum->lpVtbl->GetDefaultAudioEndpoint(
        g_cap_enum,
        eCapture,
        eConsole,
        &g_cap_device
    );

    if (FAILED(hr))
    {
        capture_release_interfaces();
        return -1;
    }

    hr = g_cap_device->lpVtbl->Activate(
        g_cap_device,
        &IID_IAudioClient,
        CLSCTX_ALL,
        NULL,
        (void**)&g_cap_audio
    );

    if (FAILED(hr))
    {
        capture_release_interfaces();
        return -1;
    }

    hr = g_cap_audio->lpVtbl->GetMixFormat(g_cap_audio, &g_cap_format);
    if (FAILED(hr) || !g_cap_format)
    {
        capture_release_interfaces();
        return -1;
    }

    hr = g_cap_audio->lpVtbl->Initialize(
        g_cap_audio,
        AUDCLNT_SHAREMODE_SHARED,
        0,
        buffer_duration,
        0,
        g_cap_format,
        NULL
    );
    if (FAILED(hr))
    {
        capture_release_interfaces();
        return -1;
    }

    hr = g_cap_audio->lpVtbl->GetService(
        g_cap_audio,
        &IID_IAudioCaptureClient,
        (void**)&g_cap_client
    );

    if (FAILED(hr))
    {
        capture_release_interfaces();
        return -1;
    }

    hr = g_cap_audio->lpVtbl->Start(g_cap_audio);
    if (FAILED(hr))
    {
        capture_release_interfaces();
        return -1;
    }

    g_capture_active = 1;

    CAP_DBG("Capture started\n");

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Capture shutdown                                                          */
/* ------------------------------------------------------------------------- */

void fb_sfxPlatformCaptureStop(void)
{
    if (g_cap_audio && g_capture_active)
        g_cap_audio->lpVtbl->Stop(g_cap_audio);

    g_capture_active = 0;
    capture_release_interfaces();

    if (g_capture_com_ready)
    {
        CoUninitialize();
        g_capture_com_ready = 0;
    }
}


/* ------------------------------------------------------------------------- */
/* Capture read                                                              */
/* ------------------------------------------------------------------------- */

/*
    Read captured frames into a buffer.

    A full implementation should:

        • call GetBuffer()
        • copy frames into the destination buffer
        • call ReleaseBuffer()
*/

int fb_sfxPlatformCaptureRead(float *buffer, int frames)
{
    int produced = 0;

    if (!buffer || frames <= 0 || !g_capture_active)
        return 0;

    while (produced < frames)
    {
        UINT32 packet_frames = 0;
        BYTE *packet = NULL;
        DWORD flags = 0;
        HRESULT hr;
        UINT32 frames_to_copy;
        int out_channels;

        hr = g_cap_client->lpVtbl->GetNextPacketSize(g_cap_client, &packet_frames);
        if (FAILED(hr) || packet_frames == 0)
            break;

        hr = g_cap_client->lpVtbl->GetBuffer(
            g_cap_client,
            &packet,
            &packet_frames,
            &flags,
            NULL,
            NULL
        );
        if (FAILED(hr))
            break;

        frames_to_copy = packet_frames;
        if (frames_to_copy > (UINT32)(frames - produced))
            frames_to_copy = (UINT32)(frames - produced);

        out_channels = (__fb_sfx && __fb_sfx->capture.channels > 0)
            ? __fb_sfx->capture.channels
            : FB_SFX_DEFAULT_CHANNELS;

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
        {
            memset(buffer + (produced * out_channels),
                   0,
                   (size_t)frames_to_copy * (size_t)out_channels * sizeof(float));
        }
        else
        {
            capture_convert_to_float(packet,
                                     buffer + (produced * out_channels),
                                     (int)frames_to_copy,
                                     g_cap_format,
                                     out_channels);
        }

        g_cap_client->lpVtbl->ReleaseBuffer(g_cap_client, packet_frames);
        produced += (int)frames_to_copy;
    }

    return produced;
}


/* ------------------------------------------------------------------------- */
/* Capture status                                                            */
/* ------------------------------------------------------------------------- */

int fb_sfxCaptureActive(void)
{
    return g_capture_active;
}


/* end of sfx_capture_wasapi.c */
