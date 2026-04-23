/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_winmm.c

    Purpose:

        Implement the Windows WinMM audio driver for sfxlib.

        WinMM (Windows Multimedia API) is the classic Windows
        audio interface available since Windows 3.x. While older
        than WASAPI and DirectSound, it is extremely stable and
        widely supported, making it a reliable fallback driver.

    Responsibilities:

        • initialize the WinMM audio subsystem
        • open a waveform output device
        • stream mixed audio buffers to the device
        • cleanly shut down the driver

    This file intentionally does NOT contain:

        • audio mixing logic
        • synthesis code
        • audio file decoding

    Architectural overview:

        sfxlib mixer
              │
        WinMM driver
              │
        Windows multimedia subsystem
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "fb_sfx_win32.h"

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif


/* ------------------------------------------------------------------------- */
/* Driver state                                                              */
/* ------------------------------------------------------------------------- */

static HWAVEOUT g_waveout = NULL;
static WAVEFORMATEX g_format;
static HANDLE g_buffer_event = NULL;
#define FB_SFX_WINMM_BUFFER_COUNT 12
static WAVEHDR g_headers[FB_SFX_WINMM_BUFFER_COUNT];
static short *g_buffers[FB_SFX_WINMM_BUFFER_COUNT] = { NULL };
static int g_buffer_samples = 0;
static int g_buffer_bytes = 0;
static int g_current_buffer = 0;
static UINT g_selected_device = WAVE_MAPPER;
static HANDLE g_worker_thread = NULL;
static DWORD g_worker_thread_id = 0;
static volatile LONG g_worker_stop = 0;
static volatile LONG g_worker_running = 0;


/* ------------------------------------------------------------------------- */
/* Debug helper                                                              */
/* ------------------------------------------------------------------------- */

static int winmm_debug_enabled(void)
{
    const char *e = getenv("SFXLIB_DEBUG");
    return (e && *e && *e != '0');
}

#define WINMM_DBG(...) \
    do { if (winmm_debug_enabled()) fprintf(stderr, "WINMM: " __VA_ARGS__); } while (0)

static void winmm_exit(void);

static int winmm_worker_frames(void)
{
    int channels = (g_format.nChannels > 0) ? (int)g_format.nChannels : FB_SFX_DEFAULT_CHANNELS;
    int frames = (channels > 0) ? (g_buffer_samples / channels) : FB_SFX_DEFAULT_BUFFER;

    frames /= 4;

    if (frames < 256)
        frames = 256;
    else if (frames > 2048)
        frames = 2048;

    return frames;
}

static DWORD WINAPI winmm_audio_worker(LPVOID unused)
{
    (void)unused;

    while (InterlockedCompareExchange(&g_worker_stop, 0, 0) == 0)
    {
        if (InterlockedCompareExchange(&g_worker_running, 0, 0) == 0)
        {
            Sleep(5);
            continue;
        }

        fb_sfxUpdate(winmm_worker_frames());
    }

    return 0;
}

static int winmm_ensure_worker(void)
{
    if (g_worker_thread)
        return 0;

    InterlockedExchange(&g_worker_stop, 0);
    g_worker_thread = CreateThread(NULL, 0, winmm_audio_worker, NULL, 0, &g_worker_thread_id);
    if (!g_worker_thread)
        return -1;

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Driver initialization                                                     */
/* ------------------------------------------------------------------------- */

static int winmm_init(int rate, int channels, int buffer_size, int flags)
{
    MMRESULT res;
    int i;

    (void)flags;

    WINMM_DBG("Initializing WinMM driver\n");

    g_format.wFormatTag      = WAVE_FORMAT_PCM;
    g_format.nChannels       = (WORD)channels;
    g_format.nSamplesPerSec  = (DWORD)rate;
    g_format.wBitsPerSample  = 16;

    g_format.nBlockAlign = (g_format.nChannels * g_format.wBitsPerSample) / 8;
    g_format.nAvgBytesPerSec = g_format.nSamplesPerSec * g_format.nBlockAlign;
    g_format.cbSize = 0;

    if (!g_buffer_event)
        g_buffer_event = CreateEvent(NULL, FALSE, FALSE, NULL);

    res = waveOutOpen(
        &g_waveout,
        g_selected_device,
        &g_format,
        (DWORD_PTR)g_buffer_event,
        0,
        g_buffer_event ? CALLBACK_EVENT : CALLBACK_NULL
    );

    if (res != MMSYSERR_NOERROR)
    {
        WINMM_DBG("waveOutOpen failed\n");
        return -1;
    }

    g_buffer_samples = buffer_size * channels;
    if (g_buffer_samples <= 0)
        g_buffer_samples = 4096;

    g_buffer_bytes = (int)(sizeof(short) * g_buffer_samples);
    g_current_buffer = 0;
    memset(g_headers, 0, sizeof(g_headers));

    for (i = 0; i < FB_SFX_WINMM_BUFFER_COUNT; ++i)
    {
        g_buffers[i] = (short *)malloc((size_t)g_buffer_bytes);
        if (!g_buffers[i])
        {
            WINMM_DBG("buffer allocation failed\n");
            winmm_exit();
            return -1;
        }

        memset(g_buffers[i], 0, (size_t)g_buffer_bytes);
        g_headers[i].lpData = (LPSTR)g_buffers[i];
        g_headers[i].dwBufferLength = (DWORD)g_buffer_bytes;

        res = waveOutPrepareHeader(g_waveout, &g_headers[i], sizeof(WAVEHDR));
        if (res != MMSYSERR_NOERROR)
        {
            WINMM_DBG("waveOutPrepareHeader failed during init\n");
            winmm_exit();
            return -1;
        }
    }

    if (winmm_ensure_worker() != 0)
    {
        WINMM_DBG("audio worker creation failed\n");
        winmm_exit();
        return -1;
    }

    InterlockedExchange(&g_worker_running, 1);

    WINMM_DBG("WinMM driver initialized\n");

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Driver shutdown                                                           */
/* ------------------------------------------------------------------------- */

static void winmm_exit(void)
{
    int i;

    InterlockedExchange(&g_worker_running, 0);
    InterlockedExchange(&g_worker_stop, 1);

    if (g_worker_thread)
    {
        if (GetCurrentThreadId() != g_worker_thread_id)
            WaitForSingleObject(g_worker_thread, INFINITE);

        CloseHandle(g_worker_thread);
        g_worker_thread = NULL;
        g_worker_thread_id = 0;
    }

    if (g_waveout)
    {
        waveOutReset(g_waveout);

        for (i = 0; i < FB_SFX_WINMM_BUFFER_COUNT; ++i)
        {
            if (g_headers[i].dwFlags & WHDR_PREPARED)
                waveOutUnprepareHeader(g_waveout, &g_headers[i], sizeof(WAVEHDR));
        }

        waveOutClose(g_waveout);
        g_waveout = NULL;
    }

    for (i = 0; i < FB_SFX_WINMM_BUFFER_COUNT; ++i)
    {
        free(g_buffers[i]);
        g_buffers[i] = NULL;
        memset(&g_headers[i], 0, sizeof(g_headers[i]));
    }

    if (g_buffer_event)
    {
        CloseHandle(g_buffer_event);
        g_buffer_event = NULL;
    }

    g_buffer_samples = 0;
    g_buffer_bytes = 0;
    g_current_buffer = 0;
}


/* ------------------------------------------------------------------------- */
/* Audio output                                                              */
/* ------------------------------------------------------------------------- */

static int winmm_write(const float *buffer, int frames)
{
    int i;
    int channels = g_format.nChannels > 0 ? g_format.nChannels : 2;
    int samples = frames * channels;
    WAVEHDR *header;
    short *dst;
    MMRESULT res;

    if (!g_waveout || !buffer || frames <= 0)
        return -1;

    if (samples > g_buffer_samples)
        samples = g_buffer_samples;

    header = &g_headers[g_current_buffer];
    dst = g_buffers[g_current_buffer];

    while (header->dwFlags & WHDR_INQUEUE)
    {
        if (g_buffer_event)
            WaitForSingleObject(g_buffer_event, 10);
        else
            Sleep(1);
    }

    for (i = 0; i < samples; i++)
    {
        float s = buffer[i];

        if (s > 1.0f)  s = 1.0f;
        if (s < -1.0f) s = -1.0f;

        dst[i] = (short)(s * 32767.0f);
    }

    header->lpData = (LPSTR)dst;
    header->dwBufferLength = (DWORD)(samples * (int)sizeof(short));
    header->dwFlags &= ~(DWORD)WHDR_DONE;

    res = waveOutWrite(g_waveout, header, sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR)
        return -1;

    g_current_buffer = (g_current_buffer + 1) % FB_SFX_WINMM_BUFFER_COUNT;

    return samples / channels;
}

static int winmm_device_list(void)
{
    return (int)waveOutGetNumDevs();
}

static int winmm_device_select(int device_id)
{
    UINT count = waveOutGetNumDevs();

    if (device_id < 0 || (UINT)device_id >= count)
        return -1;

    g_selected_device = (UINT)device_id;
    return 0;
}


/* ------------------------------------------------------------------------- */
/* Driver definition                                                         */
/* ------------------------------------------------------------------------- */

const FB_SFX_DRIVER fb_sfxDriverWinMM =
{
    "WinMM",
    0,
    winmm_init,
    winmm_exit,
    winmm_write,
    NULL,
    NULL,
    winmm_device_list,
    winmm_device_select
};


/* end of sfx_driver_winmm.c */
