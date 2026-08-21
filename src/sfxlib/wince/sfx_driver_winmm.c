/*
    FreeBASIC Sound Library support for Windows CE
    -----------------------------------------------

    File: sfx_driver_winmm.c

    Purpose:

        Implement queued PCM playback through the Windows CE WinMM waveform
        API.

    Responsibilities:

        - open and configure a waveform output device
        - convert mixed floating-point samples to signed 16-bit PCM
        - maintain a bounded queue of prepared WinMM buffers
        - feed background playback from a worker thread
        - release thread, event, buffer, and waveform resources at shutdown

    This file intentionally does NOT contain:

        - audio mixing logic
        - synthesis code
        - audio file decoding
        - native MIDI transport

    Architectural overview:

        sfxlib mixer -> PCM conversion -> WinMM buffer queue -> CE audio
*/

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"
#include "../fb_sfx_driver.h"
#include "../fb_sfx_driver_diag.h"
#include "fb_sfx_wince.h"

#include <windows.h>
#include <mmsystem.h>
#include <limits.h>
#include <stdint.h>
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

#define WINMM_DBG(...) SFX_DEBUG(__VA_ARGS__)

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
        if (__fb_sfx && __fb_sfx->shutting_down)
        {
            Sleep(5);
            continue;
        }

        if (fb_sfxForegroundFeedActive())
        {
            Sleep(5);
            continue;
        }

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

    WINMM_DBG("WinMM: initializing driver");

    if (rate <= 0 || (channels != 1 && channels != 2))
        return -1;

    if (buffer_size <= 0)
        buffer_size = FB_SFX_DEFAULT_BUFFER;

    if (buffer_size > INT_MAX / channels)
        return -1;

    g_buffer_samples = buffer_size * channels;
    if (g_buffer_samples > INT_MAX / (int)sizeof(short))
        return -1;

    g_buffer_bytes = g_buffer_samples * (int)sizeof(short);

    g_format.wFormatTag      = WAVE_FORMAT_PCM;
    g_format.nChannels       = (WORD)(unsigned int)channels;
    g_format.nSamplesPerSec  = (DWORD)(unsigned int)rate;
    g_format.wBitsPerSample  = 16;

    g_format.nBlockAlign = (WORD)(((unsigned int)g_format.nChannels *
        (unsigned int)g_format.wBitsPerSample) / 8u);
    if ((uint32_t)(unsigned int)rate >
        UINT32_MAX / (uint32_t)g_format.nBlockAlign)
        return -1;

    g_format.nAvgBytesPerSec = g_format.nSamplesPerSec * g_format.nBlockAlign;
    g_format.cbSize = 0;

    if (!g_buffer_event)
        /* Older MIPS COREDLL images omit the optional ANSI compatibility
           alias while consistently exporting the native wide entry point. */
        g_buffer_event = CreateEventW(NULL, FALSE, FALSE, NULL);

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
        WINMM_DBG("WinMM: waveOutOpen failed");
        return -1;
    }

    g_current_buffer = 0;
    memset(g_headers, 0, sizeof(g_headers));

    for (i = 0; i < FB_SFX_WINMM_BUFFER_COUNT; ++i)
    {
        g_buffers[i] = (short *)malloc((size_t)g_buffer_bytes);
        if (!g_buffers[i])
        {
            WINMM_DBG("WinMM: buffer allocation failed");
            winmm_exit();
            return -1;
        }

        memset(g_buffers[i], 0, (size_t)g_buffer_bytes);
        g_headers[i].lpData = (LPSTR)g_buffers[i];
        g_headers[i].dwBufferLength = (DWORD)g_buffer_bytes;

        res = waveOutPrepareHeader(g_waveout, &g_headers[i], sizeof(WAVEHDR));
        if (res != MMSYSERR_NOERROR)
        {
            WINMM_DBG("WinMM: waveOutPrepareHeader failed during init");
            winmm_exit();
            return -1;
        }
    }

    if (winmm_ensure_worker() != 0)
    {
        WINMM_DBG("WinMM: audio worker creation failed");
        winmm_exit();
        return -1;
    }

    InterlockedExchange(&g_worker_running, 1);

    WINMM_DBG("WinMM: driver initialized");

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
    int channels = g_format.nChannels > 0 ? g_format.nChannels : 2;
    int samples = frames * channels;
    WAVEHDR *header;
    short *dst;
    MMRESULT res;

    if (!buffer || frames <= 0)
        return -1;

    if (samples > g_buffer_samples)
        samples = g_buffer_samples;

    fb_sfxDriverDiagnostics("WinMM", buffer, samples / channels, channels);

    for (;;)
    {
        fb_sfxDriverIoLock();
        if (!g_waveout || !g_buffer_bytes)
        {
            fb_sfxDriverIoUnlock();
            return -1;
        }

        header = &g_headers[g_current_buffer];
        dst = g_buffers[g_current_buffer];
        if (!dst)
        {
            fb_sfxDriverIoUnlock();
            return -1;
        }

        if (!(header->dwFlags & WHDR_INQUEUE))
            break;

        if (InterlockedCompareExchange(&g_worker_stop, 0, 0) != 0 ||
            InterlockedCompareExchange(&g_worker_running, 0, 0) == 0)
        {
            fb_sfxDriverIoUnlock();
            return -1;
        }

        fb_sfxDriverIoUnlock();
        if (g_buffer_event)
            WaitForSingleObject(g_buffer_event, 10);
        else
            Sleep(1);
    }

    if (samples > g_buffer_samples)
        samples = g_buffer_samples;

    fb_sfxConvertFloatToS16(buffer, dst, samples);

    header->lpData = (LPSTR)dst;
    header->dwBufferLength = (DWORD)samples * (DWORD)sizeof(short);
    header->dwFlags &= ~(DWORD)WHDR_DONE;

    res = waveOutWrite(g_waveout, header, sizeof(WAVEHDR));
    if (res != MMSYSERR_NOERROR)
    {
        fb_sfxDriverIoUnlock();
        return -1;
    }

    g_current_buffer = (g_current_buffer + 1) % FB_SFX_WINMM_BUFFER_COUNT;
    fb_sfxDriverIoUnlock();

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
    FB_SFX_DRIVER_CAP_BACKGROUND,
    winmm_init,
    winmm_exit,
    winmm_write,
    NULL,
    NULL,
    winmm_device_list,
    winmm_device_select
};


/* end of sfx_driver_winmm.c */
