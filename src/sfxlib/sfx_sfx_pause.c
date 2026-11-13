/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_sfx_pause.c

    Purpose:

        Implement the SFX PAUSE command.

        This temporarily suspends sound effect playback
        without resetting the playback position.

    Responsibilities:

        • pause sound effects by identifier
        • pause sound effects by channel
        • update voice state safely

    This file intentionally does NOT contain:

        • mixer algorithms
        • audio driver interaction
        • decoding logic

    Architectural overview:

        SFX PAUSE
             │
             ▼
        voice state update
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Pause SFX by identifier                                                   */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxPause()

    Pause all voices playing a specific sound effect.
*/

void fb_sfxSfxPause(int id)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_SFX)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (!fb_sfx_context->voices[i].active)
            continue;

        if (fb_sfx_context->voices[i].type != FB_SFX_VOICE_SFX)
            continue;

        if (fb_sfx_context->voices[i].sfx_id != id)
            continue;

        fb_sfx_context->voices[i].paused = 1;
    }

    SFX_DEBUG("sfx_sfx_pause: id=%d", id);
}


/* ------------------------------------------------------------------------- */
/* Pause SFX by channel                                                      */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxPauseChannel()

    Pause all sound effects playing on a channel.
*/

void fb_sfxSfxPauseChannel(int channel)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (!fb_sfx_context->voices[i].active)
            continue;

        if (fb_sfx_context->voices[i].type != FB_SFX_VOICE_SFX)
            continue;

        if (fb_sfx_context->voices[i].channel != channel)
            continue;

        fb_sfx_context->voices[i].paused = 1;
    }

    SFX_DEBUG("sfx_sfx_pause: channel=%d", channel);
}


/* ------------------------------------------------------------------------- */
/* Pause all SFX                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxPauseAll()

    Pause every active SFX voice.
*/

void fb_sfxSfxPauseAll(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (fb_sfx_context->voices[i].active &&
            fb_sfx_context->voices[i].type == FB_SFX_VOICE_SFX)
            fb_sfx_context->voices[i].paused = 1;
    }

    SFX_DEBUG("sfx_sfx_pause: all");
}


/* end of sfx_sfx_pause.c */
