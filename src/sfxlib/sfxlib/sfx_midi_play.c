/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_play.c

    Purpose:

        Implement the MIDI PLAY command.

        This command plays a MIDI file using the currently
        opened MIDI device.

    Responsibilities:

        • open a MIDI file
        • parse the basic MIDI file structure
        • feed MIDI events to the active MIDI device

    This file intentionally does NOT contain:

        • full MIDI parser implementation
        • synthesizer logic
        • platform MIDI driver implementation

    Architectural overview:

        MIDI PLAY "file"
              │
        MIDI file reader
              │
        MIDI event stream
              │
        platform MIDI driver
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__DJGPP__)
#include <dos.h>
#else
#include <time.h>
#endif

#if FB_SFX_MT_ENABLED && !defined(_WIN32)
#include <pthread.h>
#endif


/* ------------------------------------------------------------------------- */
/* External MIDI state                                                       */
/* ------------------------------------------------------------------------- */

extern int fb_sfxMidiIsOpen(void);


/* ------------------------------------------------------------------------- */
/* Platform driver entry                                                     */
/* ------------------------------------------------------------------------- */

extern int fb_sfxMidiDriverSend(unsigned char status,
                                unsigned char data1,
                                unsigned char data2);


/* ------------------------------------------------------------------------- */
/* MIDI playback state                                                       */
/* ------------------------------------------------------------------------- */

FILE *g_midi_file   = NULL;
int   g_midi_playing = 0;

#if FB_SFX_MT_ENABLED
#if defined(_WIN32)
static HANDLE g_midi_thread = NULL;
#else
static pthread_t g_midi_thread;
#endif
static int g_midi_thread_valid = 0;
#endif

typedef struct FB_SFX_MIDI_PLAYDATA
{
    unsigned char *data;
    size_t size;
} FB_SFX_MIDI_PLAYDATA;

typedef struct FB_SFX_MIDI_TRACKSTATE
{
    const unsigned char *data;
    size_t size;
    size_t offset;
    unsigned char running_status;
    unsigned long next_tick;
    int finished;
} FB_SFX_MIDI_TRACKSTATE;

static void fb_sfxMidiSleepMs(unsigned long milliseconds)
{
    if (milliseconds == 0)
        return;

#if defined(_WIN32)
    Sleep((DWORD)milliseconds);
#elif defined(__DJGPP__)
    delay((unsigned)milliseconds);
#else
    struct timespec req;

    req.tv_sec = (time_t)(milliseconds / 1000UL);
    req.tv_nsec = (long)((milliseconds % 1000UL) * 1000000UL);
    nanosleep(&req, NULL);
#endif
}

static int fb_sfxMidiShouldContinue(void)
{
    int playing;

    fb_sfxRuntimeLock();
    playing = g_midi_playing;
    fb_sfxRuntimeUnlock();

    return playing;
}

static int fb_sfxMidiShouldPause(void)
{
    int paused;

    fb_sfxRuntimeLock();
    paused = g_midi_playing && fb_sfxMidiPaused();
    fb_sfxRuntimeUnlock();

    return paused;
}

static int fb_sfxMidiWaitMs(unsigned long milliseconds)
{
    const unsigned long quantum = 10;
    unsigned long remaining = milliseconds;

    while (remaining > 0)
    {
        unsigned long slice;

        if (!fb_sfxMidiShouldContinue())
            return -1;

        while (fb_sfxMidiShouldPause())
        {
            if (!fb_sfxMidiShouldContinue())
                return -1;

            fb_sfxMidiSleepMs(5);
        }

        slice = (remaining > quantum) ? quantum : remaining;
        fb_sfxMidiSleepMs(slice);
        remaining -= slice;
    }

    return 0;
}


static unsigned long fb_sfxReadBe32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) |
           ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) |
           (unsigned long)p[3];
}

static unsigned short fb_sfxReadBe16(const unsigned char *p)
{
    return (unsigned short)(((unsigned short)p[0] << 8) |
                             (unsigned short)p[1]);
}

static int fb_sfxMidiReadVarLen(const unsigned char *data,
                                size_t size,
                                size_t *offset,
                                unsigned long *value)
{
    unsigned long result = 0;
    int count = 0;

    if (!data || !offset || !value)
        return -1;

    do
    {
        unsigned char byte;

        if (*offset >= size || count >= 4)
            return -1;

        byte = data[*offset];
        (*offset)++;

        result = (result << 7) | (unsigned long)(byte & 0x7Fu);
        count++;

        if ((byte & 0x80u) == 0)
        {
            *value = result;
            return 0;
        }
    } while (1);
}

static int fb_sfxMidiMessageLength(unsigned char status)
{
    switch (status & 0xF0u)
    {
        case 0xC0u:
        case 0xD0u:
            return 1;
        case 0x80u:
        case 0x90u:
        case 0xA0u:
        case 0xB0u:
        case 0xE0u:
            return 2;
        default:
            return 0;
    }
}

static int fb_sfxMidiTrackPrime(FB_SFX_MIDI_TRACKSTATE *track)
{
    unsigned long delta = 0;

    if (!track)
        return -1;

    if (fb_sfxMidiReadVarLen(track->data, track->size, &track->offset, &delta) != 0)
        return -1;

    track->next_tick = delta;
    return 0;
}

static int fb_sfxMidiTrackAdvance(FB_SFX_MIDI_TRACKSTATE *track)
{
    unsigned long delta = 0;

    if (!track)
        return -1;

    if (track->offset >= track->size)
    {
        track->finished = 1;
        return 0;
    }

    if (fb_sfxMidiReadVarLen(track->data, track->size, &track->offset, &delta) != 0)
        return -1;

    track->next_tick += delta;
    return 0;
}

static int fb_sfxMidiTrackProcessEvent(FB_SFX_MIDI_TRACKSTATE *track,
                                       unsigned long *tempo_us_per_qn)
{
    unsigned char status;
    int data_len;
    unsigned char data1 = 0;
    unsigned char data2 = 0;

    if (!track || track->finished)
        return 0;

    if (track->offset >= track->size)
    {
        track->finished = 1;
        return 0;
    }

    status = track->data[track->offset];

    if ((status & 0x80u) != 0)
    {
        track->offset++;
    }
    else
    {
        if (track->running_status == 0)
            return -1;

        status = track->running_status;
    }

    if (status == 0xFFu)
    {
        unsigned char meta_type;
        unsigned long meta_len = 0;

        if (track->offset >= track->size)
            return -1;

        meta_type = track->data[track->offset];
        track->offset++;

        if (fb_sfxMidiReadVarLen(track->data, track->size, &track->offset, &meta_len) != 0)
            return -1;

        if (track->offset + (size_t)meta_len > track->size)
            return -1;

        if (meta_type == 0x51u && meta_len == 3 && tempo_us_per_qn)
        {
            *tempo_us_per_qn =
                ((unsigned long)track->data[track->offset] << 16) |
                ((unsigned long)track->data[track->offset + 1] << 8) |
                (unsigned long)track->data[track->offset + 2];
        }

        track->offset += (size_t)meta_len;
        track->running_status = 0;
    }
    else if (status == 0xF0u || status == 0xF7u)
    {
        unsigned long sysex_len = 0;

        if (fb_sfxMidiReadVarLen(track->data, track->size, &track->offset, &sysex_len) != 0)
            return -1;

        if (track->offset + (size_t)sysex_len > track->size)
            return -1;

        track->offset += (size_t)sysex_len;
        track->running_status = 0;
    }
    else
    {
        data_len = fb_sfxMidiMessageLength(status);
        if (data_len == 0)
            return -1;

        track->running_status = status;

        if (track->offset >= track->size)
            return -1;

        data1 = track->data[track->offset];
        track->offset++;

        if (data_len == 2)
        {
            if (track->offset >= track->size)
                return -1;

            data2 = track->data[track->offset];
            track->offset++;
        }

        fb_sfxMidiDriverSend(status, data1, data2);
    }

    if (track->offset >= track->size)
    {
        track->finished = 1;
        return 0;
    }

    return fb_sfxMidiTrackAdvance(track);
}

static int fb_sfxMidiPlayBuffer(const unsigned char *data, size_t size)
{
    size_t offset = 0;
    FB_SFX_MIDI_TRACKSTATE *tracks = NULL;
    unsigned short track_count;
    unsigned short format;
    unsigned short division;
    unsigned short track_index;
    unsigned long tempo_us_per_qn = 500000UL;
    unsigned long current_tick = 0;
    int result = -1;

    if (!data || size < 14)
        return -1;

    if (memcmp(data, "MThd", 4) != 0)
        return -1;

    if (fb_sfxReadBe32(data + 4) < 6)
        return -1;

    format = fb_sfxReadBe16(data + 8);
    track_count = fb_sfxReadBe16(data + 10);
    division = fb_sfxReadBe16(data + 12);
    offset = 8 + (size_t)fb_sfxReadBe32(data + 4);

    (void)format;
    if (division == 0)
        return -1;

    tracks = (FB_SFX_MIDI_TRACKSTATE *)calloc(track_count, sizeof(FB_SFX_MIDI_TRACKSTATE));
    if (!tracks)
        return -1;

    for (track_index = 0; track_index < track_count; ++track_index)
    {
        unsigned long track_size;

        if (offset + 8 > size)
            return -1;

        if (memcmp(data + offset, "MTrk", 4) != 0)
            return -1;

        track_size = fb_sfxReadBe32(data + offset + 4);
        offset += 8;

        if (offset + (size_t)track_size > size)
        {
            free(tracks);
            return -1;
        }

        tracks[track_index].data = data + offset;
        tracks[track_index].size = (size_t)track_size;
        tracks[track_index].offset = 0;
        tracks[track_index].running_status = 0;
        tracks[track_index].next_tick = 0;
        tracks[track_index].finished = 0;

        if (fb_sfxMidiTrackPrime(&tracks[track_index]) != 0)
        {
            free(tracks);
            return -1;
        }

        offset += (size_t)track_size;
    }

    while (g_midi_playing)
    {
        unsigned long next_tick = 0;
        int have_next = 0;
        unsigned long tick_delta;
        unsigned long sleep_ms;

        for (track_index = 0; track_index < track_count; ++track_index)
        {
            if (tracks[track_index].finished)
                continue;

            if (!have_next || tracks[track_index].next_tick < next_tick)
            {
                next_tick = tracks[track_index].next_tick;
                have_next = 1;
            }
        }

        if (!have_next)
        {
            result = 0;
            break;
        }

        if (next_tick > current_tick)
        {
            tick_delta = next_tick - current_tick;
            sleep_ms = (unsigned long)(((unsigned long long)tick_delta *
                                        (unsigned long long)tempo_us_per_qn) /
                                       (unsigned long long)division / 1000ULL);

            if (fb_sfxMidiWaitMs(sleep_ms) != 0)
                break;

            current_tick = next_tick;
        }

        for (track_index = 0; track_index < track_count && g_midi_playing; ++track_index)
        {
            if (tracks[track_index].finished)
                continue;

            if (tracks[track_index].next_tick != current_tick)
                continue;

            if (fb_sfxMidiTrackProcessEvent(&tracks[track_index], &tempo_us_per_qn) != 0)
            {
                free(tracks);
                return -1;
            }
        }
    }

    free(tracks);
    return result;
}


/* ------------------------------------------------------------------------- */
/* Playback worker                                                           */
/* ------------------------------------------------------------------------- */

#if FB_SFX_MT_ENABLED
#if defined(_WIN32)
static DWORD WINAPI fb_sfxMidiWorkerEntry(LPVOID param)
#else
static void *fb_sfxMidiWorkerEntry(void *param)
#endif
{
    FB_SFX_MIDI_PLAYDATA *playdata = (FB_SFX_MIDI_PLAYDATA *)param;

    if (playdata)
    {
        fb_sfxMidiPlayBuffer(playdata->data, playdata->size);

        free(playdata->data);
        playdata->data = NULL;
        free(playdata);
    }

    fb_sfxRuntimeLock();
    g_midi_playing = 0;
    fb_sfxRuntimeUnlock();

    fb_sfxMidiPauseReset();

#if defined(_WIN32)
    return 0;
#else
    return NULL;
#endif
}
#endif


/* ------------------------------------------------------------------------- */
/* Internal helpers                                                          */
/* ------------------------------------------------------------------------- */

void fb_sfxMidiStopInternal(void)
{
    if (g_midi_file)
    {
        fclose(g_midi_file);
        g_midi_file = NULL;
    }

    g_midi_playing = 0;
}

void fb_sfxMidiJoinWorker(void)
{
#if FB_SFX_MT_ENABLED
#if defined(_WIN32)
    HANDLE thread = NULL;
#else
    pthread_t thread;
#endif
    int should_join = 0;

    fb_sfxRuntimeLock();
    if (g_midi_thread_valid)
    {
        thread = g_midi_thread;
        g_midi_thread_valid = 0;
        should_join = 1;
    }
    fb_sfxRuntimeUnlock();

    if (!should_join)
        return;

#if defined(_WIN32)
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    pthread_join(thread, NULL);
#endif
#endif
}


/* ------------------------------------------------------------------------- */
/* MIDI PLAY                                                                 */
/* ------------------------------------------------------------------------- */

int fb_sfxMidiPlay(const char *filename)
{
    long file_size;
    size_t read_size;
    unsigned char *file_data;
#if FB_SFX_MT_ENABLED
    FB_SFX_MIDI_PLAYDATA *playdata;
#if defined(_WIN32)
    HANDLE thread;
#else
    pthread_t thread;
#endif
#endif

    if (!fb_sfxEnsureInitialized())
        return -1;

    if (!filename)
        return -1;

    fb_sfxMidiStop();
    fb_sfxMidiJoinWorker();

    fb_sfxRuntimeLock();
    if (!fb_sfxMidiIsOpen())
    {
        fb_sfxRuntimeUnlock();
        SFX_DEBUG("sfx_midi_play: no MIDI device open");
        return -1;
    }
    g_midi_file = fopen(filename, "rb");

    if (!g_midi_file)
    {
        fb_sfxRuntimeUnlock();
        SFX_DEBUG("sfx_midi_play: unable to open '%s'", filename);
        return -1;
    }

    if (fseek(g_midi_file, 0, SEEK_END) != 0)
    {
        fb_sfxMidiStopInternal();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    file_size = ftell(g_midi_file);
    if (file_size <= 0)
    {
        fb_sfxMidiStopInternal();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    if (fseek(g_midi_file, 0, SEEK_SET) != 0)
    {
        fb_sfxMidiStopInternal();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    file_data = (unsigned char *)malloc((size_t)file_size);
    if (!file_data)
    {
        fb_sfxMidiStopInternal();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    read_size = fread(file_data, 1, (size_t)file_size, g_midi_file);
    if (read_size != (size_t)file_size)
    {
        free(file_data);
        fb_sfxMidiStopInternal();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    fclose(g_midi_file);
    g_midi_file = NULL;

    g_midi_playing = 1;
    fb_sfxMidiPauseReset();

#if FB_SFX_MT_ENABLED
    playdata = (FB_SFX_MIDI_PLAYDATA *)malloc(sizeof(FB_SFX_MIDI_PLAYDATA));
    if (!playdata)
    {
        free(file_data);
        fb_sfxMidiStopInternal();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    playdata->data = file_data;
    playdata->size = read_size;

#if defined(_WIN32)
    thread = CreateThread(NULL, 0, fb_sfxMidiWorkerEntry, playdata, 0, NULL);
    if (!thread)
#else
    if (pthread_create(&thread, NULL, fb_sfxMidiWorkerEntry, playdata) != 0)
#endif
    {
        free(playdata->data);
        free(playdata);
        fb_sfxMidiStopInternal();
        fb_sfxRuntimeUnlock();
        return -1;
    }

    g_midi_thread = thread;
    g_midi_thread_valid = 1;
    fb_sfxRuntimeUnlock();
    return 0;
#else
    fb_sfxRuntimeUnlock();

    if (fb_sfxMidiPlayBuffer(file_data, read_size) != 0)
    {
        free(file_data);
        fb_sfxRuntimeLock();
        fb_sfxMidiStopInternal();
        fb_sfxRuntimeUnlock();
        fb_sfxMidiPauseReset();
        return -1;
    }

    free(file_data);
    fb_sfxRuntimeLock();
    fb_sfxMidiStopInternal();
    fb_sfxRuntimeUnlock();
    fb_sfxMidiPauseReset();
    return 0;
#endif
}


/* ------------------------------------------------------------------------- */
/* Status helpers                                                            */
/* ------------------------------------------------------------------------- */

int fb_sfxMidiPlaying(void)
{
    int playing;

    fb_sfxRuntimeLock();
    playing = g_midi_playing;
    fb_sfxRuntimeUnlock();

    return playing;
}


/* end of sfx_midi_play.c */
