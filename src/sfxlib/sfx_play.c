/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_play.c

    Purpose:

        Implement the BASIC PLAY command.

        PLAY interprets a music string and converts it into
        note events using the internal tone synthesis system.

    Responsibilities:

        • parse music strings (MML)
        • convert notes into frequencies
        • manage tempo and octave state
        • schedule tones on channels

    This file intentionally does NOT contain:

        • oscillator generation
        • mixer logic
        • driver interaction
        • envelope algorithms

    Architectural overview:

        PLAY string
             │
             ▼
        parser (this file)
             │
             ▼
        tone generation
             │
             ▼
        mixer → buffer → driver
*/

#include <ctype.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__DJGPP__)
#include <dos.h>
#else
#include <time.h>
#endif

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Default music state                                                       */
/* ------------------------------------------------------------------------- */

typedef struct
{
    int tempo;
    int octave;
    int default_length;
    int channel;
    int foreground;
    int articulation;
} FB_SFX_PLAYSTATE;

#define FB_SFX_PLAY_VOLUME 0.75f
#define FB_SFX_PLAY_OCTAVE_MIN 0
#define FB_SFX_PLAY_OCTAVE_MAX 6
#define FB_SFX_PLAY_TEMPO_MIN 32
#define FB_SFX_PLAY_TEMPO_MAX 255
#define FB_SFX_PLAY_NOTE_MAX 84

#define FB_SFX_PLAY_MODE_NORMAL   0
#define FB_SFX_PLAY_MODE_LEGATO   1
#define FB_SFX_PLAY_MODE_STACCATO 2

static void fb_sfxPlaySleepMs(unsigned long milliseconds)
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

static int fb_sfxPlayDurationFrames(float duration)
{
    int frames;

    if (!__fb_sfx || __fb_sfx->samplerate <= 0 || duration <= 0.0f)
        return 0;

    frames = (int)(duration * (float)__fb_sfx->samplerate + 0.5f);
    if (frames <= 0)
        frames = 1;

    return frames;
}

static int fb_sfxPlayClampOctave(int octave)
{
    if (octave < FB_SFX_PLAY_OCTAVE_MIN)
        return FB_SFX_PLAY_OCTAVE_MIN;

    if (octave > FB_SFX_PLAY_OCTAVE_MAX)
        return FB_SFX_PLAY_OCTAVE_MAX;

    return octave;
}

static int fb_sfxPlayParseNumber(const char **pp, int *value)
{
    const char *p;
    int result;
    int saw_digit;

    if (!pp || !*pp || !value)
        return 0;

    p = *pp;
    result = 0;
    saw_digit = 0;

    while (isdigit((unsigned char)*p))
    {
        result = (result * 10) + (*p - '0');
        p++;
        saw_digit = 1;
    }

    if (!saw_digit)
        return 0;

    *pp = p;
    *value = result;
    return 1;
}

static int fb_sfxPlayParseDots(const char **pp)
{
    const char *p;
    int dots;

    if (!pp || !*pp)
        return 0;

    p = *pp;
    dots = 0;

    while (*p == '.')
    {
        dots++;
        p++;
    }

    *pp = p;
    return dots;
}

static void fb_sfxPlaySkipSeparators(const char **pp)
{
    const char *p;

    if (!pp || !*pp)
        return;

    p = *pp;

    while (*p == ' ' || *p == '\t' ||
           *p == '\r' || *p == '\n' ||
           *p == ','  || *p == ';')
    {
        p++;
    }

    *pp = p;
}

static float fb_sfxPlayApplyDots(float duration, int dots)
{
    float add;

    if (duration <= 0.0f || dots <= 0)
        return duration;

    add = duration * 0.5f;

    while (dots-- > 0)
    {
        duration += add;
        add *= 0.5f;
    }

    return duration;
}

static float fb_sfxPlayArticulationScale(const FB_SFX_PLAYSTATE *st)
{
    if (!st)
        return 0.875f;

    switch (st->articulation)
    {
        case FB_SFX_PLAY_MODE_LEGATO:
            return 1.0f;

        case FB_SFX_PLAY_MODE_STACCATO:
            return 0.75f;
    }

    return 0.875f;
}

static void fb_sfxPlayQueueTone(FB_SFX_PLAYSTATE *st,
                                int frequency,
                                float duration,
                                int start_delay)
{
    FB_SFXVOICE *voice;
    int total_frames;
    int sound_frames;

    if (!st || frequency <= 0 || duration <= 0.0f)
        return;

    voice = fb_sfxVoiceAlloc();
    if (!voice)
        return;

    voice->channel = st->channel;
    voice->type = FB_SFX_VOICE_PLAY;
    voice->volume = FB_SFX_PLAY_VOLUME;
    voice->start_delay = start_delay;
    voice->hard_stop = 1;

    fb_sfxInstrumentApply(voice, st->channel, FB_SFX_WAVE_TRIANGLE, 0);
    fb_sfxVoiceSetFrequency(voice, frequency);

    total_frames = fb_sfxPlayDurationFrames(duration);
    sound_frames = fb_sfxPlayDurationFrames(duration * fb_sfxPlayArticulationScale(st));

    if (sound_frames <= 0)
        sound_frames = 1;

    if (total_frames > 0 && sound_frames > total_frames)
        sound_frames = total_frames;

    voice->length = sound_frames;

    voice->position = 0;
}


/* ------------------------------------------------------------------------- */
/* Note frequency table                                                      */
/* ------------------------------------------------------------------------- */

static const int note_table[12] =
{
    261, /* C */
    277, /* C# */
    293, /* D */
    311, /* D# */
    329, /* E */
    349, /* F */
    370, /* F# */
    392, /* G */
    415, /* G# */
    440, /* A */
    466, /* A# */
    493  /* B */
};


/* ------------------------------------------------------------------------- */
/* Duration calculation                                                      */
/* ------------------------------------------------------------------------- */

static float fb_sfxPlayDuration(FB_SFX_PLAYSTATE *st, int length)
{
    float beat;

    if (!st)
        return 0.0f;

    if (length <= 0)
        length = st->default_length;

    beat = 60.0f / (float)st->tempo;

    return beat * (4.0f / (float)length);
}


/* ------------------------------------------------------------------------- */
/* Note parsing                                                              */
/* ------------------------------------------------------------------------- */

static int fb_sfxPlayNoteIndex(char note)
{
    switch (note)
    {
        case 'C': return 0;
        case 'D': return 2;
        case 'E': return 4;
        case 'F': return 5;
        case 'G': return 7;
        case 'A': return 9;
        case 'B': return 11;
    }

    return -1;
}


/* ------------------------------------------------------------------------- */
/* Frequency calculation                                                     */
/* ------------------------------------------------------------------------- */

static int fb_sfxPlayFrequency(int note, int octave)
{
    int base;
    int shift;

    if (note < 0 || note >= 12)
        return 0;

    base = note_table[note];

    shift = octave - 4;

    while (shift > 0)
    {
        base *= 2;
        shift--;
    }

    while (shift < 0)
    {
        base /= 2;
        shift++;
    }

    return base;
}

static int fb_sfxPlayResolveFrequency(int note, int octave)
{
    while (note < 0)
    {
        note += 12;
        octave--;
    }

    while (note >= 12)
    {
        note -= 12;
        octave++;
    }

    octave = fb_sfxPlayClampOctave(octave);
    return fb_sfxPlayFrequency(note, octave);
}

static int fb_sfxPlayNoteNumberFrequency(int note_number)
{
    int octave;
    int note;

    if (note_number <= 0 || note_number > FB_SFX_PLAY_NOTE_MAX)
        return 0;

    note = (note_number - 1) % 12;
    octave = (note_number - 1) / 12;

    return fb_sfxPlayFrequency(note, octave);
}

static void fb_sfxPlayRunForeground(int frames)
{
    int step;
    int tick_frames;
    int samplerate;

    if (frames <= 0)
        return;

    samplerate = (__fb_sfx && __fb_sfx->samplerate > 0)
        ? __fb_sfx->samplerate
        : 0;

    tick_frames = (samplerate > 0)
        ? (samplerate / 200)
        : 220;

    if (tick_frames <= 0)
        tick_frames = 220;

    while (frames > 0)
    {
        unsigned long milliseconds;

        step = (frames > tick_frames) ? tick_frames : frames;
        fb_sfxUpdate(step);

        if (samplerate > 0)
        {
            milliseconds = (unsigned long)(((unsigned long long)step * 1000ULL) / (unsigned long long)samplerate);
            if (milliseconds == 0)
                milliseconds = 1;
            fb_sfxPlaySleepMs(milliseconds);
        }

        frames -= step;
    }
}


/* ------------------------------------------------------------------------- */
/* PLAY string parser                                                        */
/* ------------------------------------------------------------------------- */

static void fb_sfxPlayStringImpl(int channel, const char *str, int default_foreground)
{
    FB_SFX_PLAYSTATE st;
    const char *p;
    int resolved_channel;
    int sequence_delay;
    int total_frames;

    if (!fb_sfxEnsureInitialized())
        return;

    if (!str)
        return;

    resolved_channel = fb_sfxResolveChannel(channel);

    st.tempo = (__fb_sfx && __fb_sfx->tempo > 0)
        ? __fb_sfx->tempo
        : 120;
    st.octave = (__fb_sfx)
        ? fb_sfxPlayClampOctave(__fb_sfx->octave)
        : 4;
    st.default_length = 4;
    st.channel = resolved_channel;
    st.foreground = default_foreground ? 1 : 0;
    if (__fb_sfx && __fb_sfx->channel_context_active)
        st.foreground = 0;
    st.articulation = FB_SFX_PLAY_MODE_NORMAL;
    sequence_delay = 0;
    total_frames = 0;

    p = str;

    while (*p)
    {
        char c;

        fb_sfxPlaySkipSeparators(&p);
        if (!*p)
            break;

        c = (char)toupper((unsigned char)*p);

        /* ------------------------------------------------------------- */
        /* Note                                                          */
        /* ------------------------------------------------------------- */

        if (c >= 'A' && c <= 'G')
        {
            float duration;
            int idx = fb_sfxPlayNoteIndex(c);
            int length = 0;
            int dots;
            int freq;
            int octave = st.octave;

            p++;
            if (*p == '#' || *p == '+')
            {
                idx++;
                p++;
            }
            else if (*p == '-')
            {
                idx--;
                p++;
            }

            (void)fb_sfxPlayParseNumber(&p, &length);
            dots = fb_sfxPlayParseDots(&p);

            freq = fb_sfxPlayResolveFrequency(idx, octave);
            duration = fb_sfxPlayApplyDots(fb_sfxPlayDuration(&st, length), dots);

            fb_sfxPlayQueueTone(&st, freq, duration, sequence_delay);

            length = fb_sfxPlayDurationFrames(duration);
            sequence_delay += length;
            total_frames += length;

            continue;
        }

        /* ------------------------------------------------------------- */
        /* Pause / rest                                                  */
        /* ------------------------------------------------------------- */

        if (c == 'P' || c == 'R')
        {
            float duration;
            int length = 0;
            int dots;

            p++;
            (void)fb_sfxPlayParseNumber(&p, &length);
            dots = fb_sfxPlayParseDots(&p);

            duration = fb_sfxPlayApplyDots(fb_sfxPlayDuration(&st, length), dots);
            length = fb_sfxPlayDurationFrames(duration);
            sequence_delay += length;
            total_frames += length;

            continue;
        }

        /* ------------------------------------------------------------- */
        /* Absolute note number                                          */
        /* ------------------------------------------------------------- */

        if (c == 'N')
        {
            float duration;
            int note_number = 0;
            int dots;
            int freq;
            int length;

            p++;
            if (!fb_sfxPlayParseNumber(&p, &note_number))
                continue;

            dots = fb_sfxPlayParseDots(&p);
            duration = fb_sfxPlayApplyDots(fb_sfxPlayDuration(&st, 0), dots);
            length = fb_sfxPlayDurationFrames(duration);

            if (note_number == 0)
            {
                sequence_delay += length;
                total_frames += length;
                continue;
            }

            freq = fb_sfxPlayNoteNumberFrequency(note_number);
            if (freq > 0)
                fb_sfxPlayQueueTone(&st, freq, duration, sequence_delay);

            sequence_delay += length;
            total_frames += length;
            continue;
        }

        /* ------------------------------------------------------------- */
        /* Octave                                                        */
        /* ------------------------------------------------------------- */

        if (c == 'O')
        {
            int octave = 0;

            p++;
            if (!fb_sfxPlayParseNumber(&p, &octave))
                continue;

            st.octave = fb_sfxPlayClampOctave(octave);
            continue;
        }

        if (c == '>')
        {
            p++;
            st.octave = fb_sfxPlayClampOctave(st.octave + 1);
            continue;
        }

        if (c == '<')
        {
            p++;
            st.octave = fb_sfxPlayClampOctave(st.octave - 1);

            continue;
        }

        /* ------------------------------------------------------------- */
        /* Tempo                                                         */
        /* ------------------------------------------------------------- */

        if (c == 'T')
        {
            int tempo = 0;

            p++;
            if (!fb_sfxPlayParseNumber(&p, &tempo))
                continue;

            if (tempo < FB_SFX_PLAY_TEMPO_MIN)
                tempo = FB_SFX_PLAY_TEMPO_MIN;
            if (tempo > FB_SFX_PLAY_TEMPO_MAX)
                tempo = FB_SFX_PLAY_TEMPO_MAX;
            st.tempo = tempo;

            continue;
        }

        /* ------------------------------------------------------------- */
        /* Default note length                                           */
        /* ------------------------------------------------------------- */

        if (c == 'L')
        {
            int length = 0;

            p++;
            if (!fb_sfxPlayParseNumber(&p, &length))
                continue;

            if (length > 0)
                st.default_length = length;

            continue;
        }

        /* ------------------------------------------------------------- */
        /* Music / execution mode                                         */
        /* ------------------------------------------------------------- */

        if (c == 'M')
        {
            p++;

            switch ((char)toupper((unsigned char)*p))
            {
                case 'B':
                    st.foreground = 0;
                    p++;
                    break;

                case 'F':
                    st.foreground = 1;
                    p++;
                    break;

                case 'L':
                    st.articulation = FB_SFX_PLAY_MODE_LEGATO;
                    p++;
                    break;

                case 'N':
                    st.articulation = FB_SFX_PLAY_MODE_NORMAL;
                    p++;
                    break;

                case 'S':
                    st.articulation = FB_SFX_PLAY_MODE_STACCATO;
                    p++;
                    break;
            }

            continue;
        }

        p++;
    }

    if (st.foreground && total_frames > 0)
        fb_sfxPlayRunForeground(total_frames);
}

void fb_sfxPlayString(int channel, const char *str)
{
    fb_sfxPlayStringImpl(channel, str, 0);
}


/* ------------------------------------------------------------------------- */
/* PLAY command                                                              */
/* ------------------------------------------------------------------------- */

void fb_sfxPlay(const char *music)
{
    fb_sfxPlayStringImpl(-1, music, 1);
}


void fb_sfxPlayChannel(int channel, const char *music)
{
    fb_sfxPlayStringImpl(channel, music, 0);
}


/* end of sfx_play.c */
