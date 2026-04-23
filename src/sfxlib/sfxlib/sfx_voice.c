/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_voice.c

    Purpose:

        Implement voice management for the FreeBASIC sound subsystem.

        A voice represents an active sound source such as:

            • generated tone
            • waveform playback
            • streamed audio
            • sound effect

    Responsibilities:

        • voice allocation
        • voice initialization
        • voice state updates
        • voice termination

    This file intentionally does NOT contain:

        • audio mixing logic
        • platform driver code
        • command parsing
        • buffer management

    Architectural overview:

        command layer
              │
              ▼
        voice management
              │
              ▼
        mixer subsystem
*/

#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Shared status values                                                      */
/* ------------------------------------------------------------------------- */

#define FB_SFX_VOICE_STATUS_STOPPED 0
#define FB_SFX_VOICE_STATUS_PLAYING 1
#define FB_SFX_VOICE_STATUS_PAUSED  2


/* ------------------------------------------------------------------------- */
/* Voice initialization                                                      */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxVoiceInit()

    Reset a voice structure to a known default state.
*/

void fb_sfxVoiceInit(FB_SFXVOICE *v)
{
    if (!v)
        return;

    memset(v, 0, sizeof(FB_SFXVOICE));

    v->active = 0;
    v->channel = 0;
    v->volume = 1.0f;
    v->pan = 0.0f;

    v->frequency = 0;

    v->position = 0;
    v->pos = 0;
    v->length = 0;
    v->start_delay = 0;
    v->hard_stop = 0;

    v->data = NULL;
}


/* ------------------------------------------------------------------------- */
/* Voice allocation                                                          */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxVoiceAlloc()

    Allocate a free voice slot from the runtime voice pool.

    Returns:

        pointer to the allocated voice
        NULL if no voices are available
*/

FB_SFXVOICE *fb_sfxVoiceAlloc(void)
{
    int i;
    FB_SFXVOICE *result = NULL;

    if (!fb_sfxEnsureInitialized())
        return NULL;

    fb_sfxRuntimeLock();

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        FB_SFXVOICE *v = &__fb_sfx->voices[i];

        if (!v->active)
        {
            fb_sfxVoiceInit(v);

            v->active = 1;

            SFX_DEBUG("sfx_voice: allocated voice %d", i);

            result = v;
            break;
        }
    }

    if (!result)
        SFX_DEBUG("sfx_voice: no free voices available");

    fb_sfxRuntimeUnlock();
    return result;
}


/* ------------------------------------------------------------------------- */
/* Voice release                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxVoiceFree()

    Release a voice slot and mark it inactive.
*/

void fb_sfxVoiceFree(FB_SFXVOICE *v)
{
    if (!v)
        return;

    fb_sfxRuntimeLock();
    v->active = 0;
    SFX_DEBUG("sfx_voice: voice released");
    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Voice control                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxVoiceStartTone()

    Configure a voice to generate a tone.
*/

void fb_sfxVoiceStartTone(FB_SFXVOICE *v, int freq, float volume)
{
    if (!v)
        return;

    v->frequency = freq;
    v->volume = volume;

    v->position = 0;
}


/*
    fb_sfxVoiceStartSample()

    Configure a voice to play a sample buffer.
*/

void fb_sfxVoiceStartSample(
    FB_SFXVOICE *v,
    const float *data,
    int length,
    float volume
)
{
    if (!v)
        return;

    v->data = data;
    v->length = length;
    v->volume = volume;

    v->position = 0;
}


/*
    fb_sfxVoiceStop()

    Stop playback for a voice.
*/

void fb_sfxVoiceStop(FB_SFXVOICE *v)
{
    if (!v)
        return;

    fb_sfxRuntimeLock();
    v->active = 0;
    SFX_DEBUG("sfx_voice: voice stopped");
    fb_sfxRuntimeUnlock();
}


/* ------------------------------------------------------------------------- */
/* Voice updates                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxVoiceAdvance()

    Advance the playback position of a voice.

    This function is typically called by the mixer after a sample
    has been generated.
*/

void fb_sfxVoiceAdvance(FB_SFXVOICE *v)
{
    if (!v)
        return;

    if (v->data)
    {
        v->position++;

        if (v->position >= v->length)
        {
            v->active = 0;
        }
    }
}


/*
    fb_sfxVoiceActiveCount()

    Return the number of currently active voices.
*/

int fb_sfxVoiceActiveCount(void)
{
    int i;
    int count = 0;

    if (!__fb_sfx)
        return 0;

    fb_sfxRuntimeLock();
    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (__fb_sfx->voices[i].active)
            count++;
    }
    fb_sfxRuntimeUnlock();

    return count;
}


/* ------------------------------------------------------------------------- */
/* Voice type helpers                                                        */
/* ------------------------------------------------------------------------- */

static int fb_sfxVoiceMatchesTypeChannel(const FB_SFXVOICE *voice,
                                         int voice_type,
                                         int channel)
{
    if (!voice)
        return 0;

    if (!voice->active)
        return 0;

    if (voice->type != voice_type)
        return 0;

    if (channel >= 0 && voice->channel != channel)
        return 0;

    return 1;
}

void fb_sfxVoiceStopType(int voice_type)
{
    fb_sfxVoiceStopTypeChannel(voice_type, -1);
}

void fb_sfxVoiceStopTypeChannel(int voice_type, int channel)
{
    int i;

    if (!__fb_sfx)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (!fb_sfxVoiceMatchesTypeChannel(voice, voice_type, channel))
            continue;

        voice->active = 0;
    }
}

void fb_sfxVoicePauseType(int voice_type)
{
    fb_sfxVoicePauseTypeChannel(voice_type, -1);
}

void fb_sfxVoicePauseTypeChannel(int voice_type, int channel)
{
    int i;

    if (!__fb_sfx)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (!fb_sfxVoiceMatchesTypeChannel(voice, voice_type, channel))
            continue;

        voice->paused = 1;
    }
}

void fb_sfxVoiceResumeType(int voice_type)
{
    fb_sfxVoiceResumeTypeChannel(voice_type, -1);
}

void fb_sfxVoiceResumeTypeChannel(int voice_type, int channel)
{
    int i;

    if (!__fb_sfx)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (!fb_sfxVoiceMatchesTypeChannel(voice, voice_type, channel))
            continue;

        voice->paused = 0;
    }
}

int fb_sfxVoiceStatusType(int voice_type)
{
    return fb_sfxVoiceStatusTypeChannel(voice_type, -1);
}

int fb_sfxVoiceStatusTypeChannel(int voice_type, int channel)
{
    int i;
    int playing;
    int paused;

    if (!__fb_sfx)
        return FB_SFX_VOICE_STATUS_STOPPED;

    playing = 0;
    paused = 0;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (!fb_sfxVoiceMatchesTypeChannel(voice, voice_type, channel))
            continue;

        if (voice->paused)
            paused = 1;
        else
            playing = 1;
    }

    if (playing)
        return FB_SFX_VOICE_STATUS_PLAYING;

    if (paused)
        return FB_SFX_VOICE_STATUS_PAUSED;

    return FB_SFX_VOICE_STATUS_STOPPED;
}


/* end of sfx_voice.c */
