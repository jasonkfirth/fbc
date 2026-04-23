/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_sfx_status.c

    Purpose:

        Implement the SFX STATUS command.

        This allows applications to query whether a sound
        effect is currently playing, paused, or stopped.

    Responsibilities:

        • inspect the voice table
        • determine the state of a sound effect
        • provide helper queries for channels

    This file intentionally does NOT contain:

        • mixer algorithms
        • audio driver interaction
        • playback logic

    Architectural overview:

        SFX STATUS
             │
             ▼
        voice table inspection
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Status values                                                             */
/* ------------------------------------------------------------------------- */

#define FB_SFX_SFX_STOPPED 0
#define FB_SFX_SFX_PLAYING 1
#define FB_SFX_SFX_PAUSED  2


/* ------------------------------------------------------------------------- */
/* Query SFX status                                                          */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxStatus()

    Determine the current state of a sound effect.

    Returns:

        FB_SFX_SFX_STOPPED
        FB_SFX_SFX_PLAYING
        FB_SFX_SFX_PAUSED
*/

int fb_sfxSfxStatus(int id)
{
    int i;
    int playing = 0;
    int paused  = 0;

    if (!__fb_sfx)
        return FB_SFX_SFX_STOPPED;

    if (id < 0 || id >= FB_SFX_MAX_SFX)
        return FB_SFX_SFX_STOPPED;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (!__fb_sfx->voices[i].active)
            continue;

        if (__fb_sfx->voices[i].type != FB_SFX_VOICE_SFX)
            continue;

        if (__fb_sfx->voices[i].sfx_id != id)
            continue;

        if (__fb_sfx->voices[i].paused)
            paused = 1;
        else
            playing = 1;
    }

    if (playing)
        return FB_SFX_SFX_PLAYING;

    if (paused)
        return FB_SFX_SFX_PAUSED;

    return FB_SFX_SFX_STOPPED;
}


/* ------------------------------------------------------------------------- */
/* Query SFX status by channel                                               */
/* ------------------------------------------------------------------------- */

int fb_sfxSfxStatusChannel(int channel)
{
    int i;
    int playing = 0;
    int paused  = 0;

    if (!__fb_sfx)
        return FB_SFX_SFX_STOPPED;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return FB_SFX_SFX_STOPPED;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (!__fb_sfx->voices[i].active)
            continue;

        if (__fb_sfx->voices[i].type != FB_SFX_VOICE_SFX)
            continue;

        if (__fb_sfx->voices[i].channel != channel)
            continue;

        if (__fb_sfx->voices[i].paused)
            paused = 1;
        else
            playing = 1;
    }

    if (playing)
        return FB_SFX_SFX_PLAYING;

    if (paused)
        return FB_SFX_SFX_PAUSED;

    return FB_SFX_SFX_STOPPED;
}


/* ------------------------------------------------------------------------- */
/* Query if any SFX active                                                   */
/* ------------------------------------------------------------------------- */

int fb_sfxSfxAnyActive(void)
{
    int i;

    if (!__fb_sfx)
        return 0;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (__fb_sfx->voices[i].active &&
            __fb_sfx->voices[i].type == FB_SFX_VOICE_SFX)
            return 1;
    }

    return 0;
}


/* end of sfx_sfx_status.c */
