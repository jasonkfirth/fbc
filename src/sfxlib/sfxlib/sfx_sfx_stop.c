/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_sfx_stop.c

    Purpose:

        Implement the SFX STOP command.

        This stops active sound effects by terminating the
        voices associated with them.

    Responsibilities:

        • stop sound effects by identifier
        • stop sound effects by channel
        • safely terminate active voices

    This file intentionally does NOT contain:

        • mixer algorithms
        • audio driver interaction
        • decoding logic

    Architectural overview:

        SFX STOP
             │
             ▼
        voice termination
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Stop SFX by identifier                                                    */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxStop()

    Stop all voices playing a specific sound effect.
*/

void fb_sfxSfxStop(int id)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_SFX)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (!__fb_sfx->voices[i].active)
            continue;

        if (__fb_sfx->voices[i].type != FB_SFX_VOICE_SFX)
            continue;

        if (__fb_sfx->voices[i].sfx_id != id)
            continue;

        __fb_sfx->voices[i].active = 0;
    }

    SFX_DEBUG("sfx_sfx_stop: id=%d", id);
}


/* ------------------------------------------------------------------------- */
/* Stop SFX by channel                                                       */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxStopChannel()

    Stop all sound effects playing on a channel.
*/

void fb_sfxSfxStopChannel(int channel)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (!__fb_sfx->voices[i].active)
            continue;

        if (__fb_sfx->voices[i].type != FB_SFX_VOICE_SFX)
            continue;

        if (__fb_sfx->voices[i].channel != channel)
            continue;

        __fb_sfx->voices[i].active = 0;
    }

    SFX_DEBUG("sfx_sfx_stop: channel=%d", channel);
}


/* ------------------------------------------------------------------------- */
/* Stop all SFX                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxStopAll()

    Terminate all active sound effect voices.
*/

void fb_sfxSfxStopAll(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (__fb_sfx->voices[i].active &&
            __fb_sfx->voices[i].type == FB_SFX_VOICE_SFX)
            __fb_sfx->voices[i].active = 0;
    }

    SFX_DEBUG("sfx_sfx_stop: all");
}


/* end of sfx_sfx_stop.c */
