/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_wasapi.c

    Purpose:

        Implement the Windows WASAPI audio driver for sfxlib.

        WASAPI (Windows Audio Session API) is the modern low-latency
        audio interface available on Windows Vista and later. It
        provides direct access to the system audio engine and allows
        applications to stream audio efficiently.

    Responsibilities:

        • initialize the WASAPI audio subsystem
        • open the default audio endpoint
        • create a streaming client
        • send mixed audio buffers to the device
        • cleanly shut down the driver

    This file intentionally does NOT contain:

        • audio mixing logic
        • synthesis code
        • audio file decoding

    Architectural overview:

        sfxlib mixer
              │
        WASAPI driver
              │
        Windows Audio Engine
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_win32.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Driver state                                                              */
/* ------------------------------------------------------------------------- */

extern const FB_SFX_DRIVER fb_sfxDriverWinMM;
extern const FB_SFX_DRIVER __fb_sfxDriverNull;

static IMMDeviceEnumerator *g_device_enum = NULL;
static IMMDevice *g_device = NULL;
static IAudioClient *g_audio = NULL;
static IAudioRenderClient *g_render = NULL;
static WAVEFORMATEX *g_mix_format = NULL;
static UINT32 g_buffer_frames = 0;
static int g_wasapi_running = 0;
static int g_wasapi_com_ready = 0;
static HANDLE g_worker_thread = NULL;
static DWORD g_worker_thread_id = 0;
static volatile LONG g_worker_stop = 0;
static const GUID sfx_wasapi_float_guid =
{
    0x00000003, 0x0000, 0x0010,
    { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 }
};


/* ------------------------------------------------------------------------- */
/* Debug helper                                                              */
/* ------------------------------------------------------------------------- */

static int wasapi_debug_enabled(void)
{
    const char *e = getenv("SFXLIB_DEBUG");
    return (e && *e && *e != '0');
}

#define WASAPI_DBG(...) \
    do { if (wasapi_debug_enabled()) fprintf(stderr, "WASAPI: " __VA_ARGS__); } while (0)

static void wasapi_release_interfaces(void)
{
    if (g_render)
    {
        g_render->lpVtbl->Release(g_render);
        g_render = NULL;
    }

    if (g_audio)
    {
        g_audio->lpVtbl->Release(g_audio);
        g_audio = NULL;
    }

    if (g_device)
    {
        g_device->lpVtbl->Release(g_device);
        g_device = NULL;
    }

    if (g_device_enum)
    {
        g_device_enum->lpVtbl->Release(g_device_enum);
        g_device_enum = NULL;
    }

    if (g_mix_format)
    {
        CoTaskMemFree(g_mix_format);
        g_mix_format = NULL;
    }
}

static int wasapi_worker_frames(void)
{
    int frames = (g_buffer_frames > 0) ? (int)g_buffer_frames : FB_SFX_DEFAULT_BUFFER;

    frames /= 4;

    if (frames < 256)
        frames = 256;
    else if (frames > 2048)
        frames = 2048;

    return frames;
}

static DWORD WINAPI wasapi_audio_worker(LPVOID unused)
{
    HRESULT hr;
    int com_ready = 0;

    (void)unused;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
        com_ready = 1;

    while (InterlockedCompareExchange(&g_worker_stop, 0, 0) == 0)
    {
        if (!g_wasapi_running)
        {
            Sleep(5);
            continue;
        }

        fb_sfxUpdate(wasapi_worker_frames());
    }

    if (com_ready)
        CoUninitialize();

    return 0;
}

static int wasapi_ensure_worker(void)
{
    if (g_worker_thread)
        return 0;

    InterlockedExchange(&g_worker_stop, 0);
    g_worker_thread = CreateThread(NULL, 0, wasapi_audio_worker, NULL, 0, &g_worker_thread_id);
    if (!g_worker_thread)
        return -1;

    return 0;
}

static int wasapi_format_is_float(const WAVEFORMATEX *fmt)
{
    if (!fmt)
        return 0;

    if (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        return 1;

    if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        const WAVEFORMATEXTENSIBLE *ext = (const WAVEFORMATEXTENSIBLE *)fmt;
        return IsEqualGUID(&ext->SubFormat, &sfx_wasapi_float_guid);
    }

    return 0;
}

static int wasapi_bytes_per_sample(const WAVEFORMATEX *fmt)
{
    if (!fmt || fmt->nChannels == 0)
        return 0;

    return (int)(fmt->nBlockAlign / fmt->nChannels);
}

static void wasapi_convert_float_to_device(const float *src,
                                           BYTE *dst,
                                           int frames,
                                           int src_channels,
                                           const WAVEFORMATEX *fmt)
{
    int frame;
    int dst_channels;
    int bytes_per_sample;
    int use_float;

    if (!src || !dst || !fmt || frames <= 0)
        return;

    dst_channels = fmt->nChannels > 0 ? (int)fmt->nChannels : 2;
    bytes_per_sample = wasapi_bytes_per_sample(fmt);
    use_float = wasapi_format_is_float(fmt);

    for (frame = 0; frame < frames; ++frame)
    {
        float left = src[frame * src_channels];
        float right = (src_channels > 1) ? src[(frame * src_channels) + 1] : left;
        int ch;

        for (ch = 0; ch < dst_channels; ++ch)
        {
            float sample;
            BYTE *out = dst + (((frame * dst_channels) + ch) * bytes_per_sample);

            if (dst_channels == 1)
                sample = (left + right) * 0.5f;
            else if (ch == 0)
                sample = left;
            else if (ch == 1)
                sample = right;
            else
                sample = 0.0f;

            if (sample > 1.0f)
                sample = 1.0f;
            if (sample < -1.0f)
                sample = -1.0f;

            if (use_float && bytes_per_sample == (int)sizeof(float))
            {
                ((float *)dst)[(frame * dst_channels) + ch] = sample;
            }
            else if (!use_float && bytes_per_sample == 2)
            {
                short pcm = (short)(sample * 32767.0f);
                memcpy(out, &pcm, sizeof(pcm));
            }
            else if (!use_float && bytes_per_sample == 4)
            {
                int pcm = (int)(sample * 2147483647.0f);
                memcpy(out, &pcm, sizeof(pcm));
            }
            else
            {
                memset(out, 0, (size_t)bytes_per_sample);
            }
        }
    }
}


/* ------------------------------------------------------------------------- */
/* Driver initialization                                                     */
/* ------------------------------------------------------------------------- */

static int wasapi_init(int rate, int channels, int buffer_size, int flags)
{
    HRESULT hr;
    REFERENCE_TIME buffer_duration;

    (void)flags;

    WASAPI_DBG("Initializing WASAPI driver\n");

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return -1;

    if (SUCCEEDED(hr))
        g_wasapi_com_ready = 1;

    wasapi_release_interfaces();
    g_wasapi_running = 0;

    hr = CoCreateInstance(
        &CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        &IID_IMMDeviceEnumerator,
        (void **)&g_device_enum
    );

    if (FAILED(hr))
    {
        wasapi_release_interfaces();
        return -1;
    }

    hr = g_device_enum->lpVtbl->GetDefaultAudioEndpoint(
        g_device_enum,
        eRender,
        eConsole,
        &g_device
    );

    if (FAILED(hr))
    {
        wasapi_release_interfaces();
        return -1;
    }

    hr = g_device->lpVtbl->Activate(
        g_device,
        &IID_IAudioClient,
        CLSCTX_ALL,
        NULL,
        (void **)&g_audio
    );

    if (FAILED(hr))
    {
        wasapi_release_interfaces();
        return -1;
    }

    hr = g_audio->lpVtbl->GetMixFormat(g_audio, &g_mix_format);
    if (FAILED(hr) || !g_mix_format)
    {
        wasapi_release_interfaces();
        return -1;
    }

    if (g_mix_format->nChannels <= 0)
        g_mix_format->nChannels = (WORD)(channels > 0 ? channels : 2);
    if (g_mix_format->nSamplesPerSec == 0)
        g_mix_format->nSamplesPerSec = (DWORD)(rate > 0 ? rate : FB_SFX_DEFAULT_RATE);

    /*
        Keep the runtime clock aligned with the real device clock.
        Generator tones and note durations are based on __fb_sfx->samplerate,
        so leaving it at the default while WASAPI picked a different mix rate
        makes audible timing and pitch feel wrong.
    */
    if (__fb_sfx && g_mix_format->nSamplesPerSec > 0)
        __fb_sfx->samplerate = (int)g_mix_format->nSamplesPerSec;

    buffer_duration = (REFERENCE_TIME)((10000000.0 * (double)buffer_size) /
                                       (double)g_mix_format->nSamplesPerSec);
    if (buffer_duration <= 0)
        buffer_duration = 100000;

    hr = g_audio->lpVtbl->Initialize(
        g_audio,
        AUDCLNT_SHAREMODE_SHARED,
        0,
        buffer_duration,
        0,
        g_mix_format,
        NULL
    );
    if (FAILED(hr))
    {
        wasapi_release_interfaces();
        return -1;
    }

    hr = g_audio->lpVtbl->GetService(
        g_audio,
        &IID_IAudioRenderClient,
        (void **)&g_render
    );
    if (FAILED(hr))
    {
        wasapi_release_interfaces();
        return -1;
    }

    hr = g_audio->lpVtbl->GetBufferSize(g_audio, &g_buffer_frames);
    if (FAILED(hr) || g_buffer_frames == 0)
    {
        wasapi_release_interfaces();
        return -1;
    }

    hr = g_audio->lpVtbl->Start(g_audio);
    if (FAILED(hr))
    {
        wasapi_release_interfaces();
        return -1;
    }

    if (wasapi_ensure_worker() != 0)
    {
        g_audio->lpVtbl->Stop(g_audio);
        wasapi_release_interfaces();
        return -1;
    }

    g_wasapi_running = 1;

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Driver shutdown                                                           */
/* ------------------------------------------------------------------------- */

static void wasapi_exit(void)
{
    WASAPI_DBG("Shutting down WASAPI driver\n");

    g_wasapi_running = 0;
    InterlockedExchange(&g_worker_stop, 1);

    if (g_worker_thread)
    {
        if (GetCurrentThreadId() != g_worker_thread_id)
            WaitForSingleObject(g_worker_thread, INFINITE);

        CloseHandle(g_worker_thread);
        g_worker_thread = NULL;
        g_worker_thread_id = 0;
    }

    if (g_audio)
        g_audio->lpVtbl->Stop(g_audio);

    g_buffer_frames = 0;
    wasapi_release_interfaces();

    if (g_wasapi_com_ready)
    {
        CoUninitialize();
        g_wasapi_com_ready = 0;
    }
}


/* ------------------------------------------------------------------------- */
/* Audio output                                                              */
/* ------------------------------------------------------------------------- */

static int wasapi_write(const float *buffer, int frames)
{
    UINT32 padding = 0;
    UINT32 available;
    UINT32 frames_to_write;
    BYTE *dst = NULL;
    HRESULT hr;
    int src_channels;
    int total_written = 0;

    if (!g_audio || !g_render || !g_mix_format || !buffer || frames <= 0)
        return -1;

    src_channels = (__fb_sfx && __fb_sfx->output_channels > 0)
        ? __fb_sfx->output_channels
        : FB_SFX_DEFAULT_CHANNELS;

    while (total_written < frames)
    {
        hr = g_audio->lpVtbl->GetCurrentPadding(g_audio, &padding);
        if (FAILED(hr))
            return (total_written > 0) ? total_written : -1;

        if (padding >= g_buffer_frames)
        {
            Sleep(1);
            continue;
        }

        available = g_buffer_frames - padding;
        frames_to_write = (UINT32)(((frames - total_written) < (int)available)
            ? (frames - total_written)
            : (int)available);

        if (frames_to_write == 0)
        {
            Sleep(1);
            continue;
        }

        hr = g_render->lpVtbl->GetBuffer(g_render, frames_to_write, &dst);
        if (FAILED(hr) || !dst)
            return (total_written > 0) ? total_written : -1;

        wasapi_convert_float_to_device(
            buffer + ((size_t)total_written * (size_t)src_channels),
            dst,
            (int)frames_to_write,
            src_channels,
            g_mix_format);

        hr = g_render->lpVtbl->ReleaseBuffer(g_render, frames_to_write, 0);
        if (FAILED(hr))
            return (total_written > 0) ? total_written : -1;

        total_written += (int)frames_to_write;
    }

    return total_written;
}


/* ------------------------------------------------------------------------- */
/* Driver definition                                                         */
/* ------------------------------------------------------------------------- */

const FB_SFX_DRIVER fb_sfxDriverWASAPI =
{
    "WASAPI",
    0,
    wasapi_init,
    wasapi_exit,
    wasapi_write,
    NULL,
    NULL,
    NULL,
    NULL
};


/* ------------------------------------------------------------------------- */
/* Driver registry                                                           */
/* ------------------------------------------------------------------------- */

const FB_SFX_DRIVER *__fb_sfx_drivers_list[] =
{
    &fb_sfxDriverWASAPI,
    &fb_sfxDriverWinMM,
    &__fb_sfxDriverNull,
    NULL
};


/* end of sfx_driver_wasapi.c */
