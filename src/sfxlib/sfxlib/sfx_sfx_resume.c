/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_sfx_resume.c

    Purpose:

        Implement the SFX RESUME command.

        This resumes playback of sound effects that were
        previously paused using SFX PAUSE.

    Responsibilities:

        • resume sound effects by identifier
        • resume sound effects by channel
        • update voice state safely

    This file intentionally does NOT contain:

        • mixer algorithms
        • audio driver interaction
        • decoding logic

    Architectural overview:

        SFX RESUME
             │
             ▼
        voice state update
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Resume SFX by identifier                                                  */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxResume()

    Resume all paused voices playing a specific sound effect.
*/

void fb_sfxSfxResume(int id)
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

        __fb_sfx->voices[i].paused = 0;
    }

    SFX_DEBUG("sfx_sfx_resume: id=%d", id);
}


/* ------------------------------------------------------------------------- */
/* Resume SFX by channel                                                     */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxResumeChannel()

    Resume all paused sound effects on a channel.
*/

void fb_sfxSfxResumeChannel(int channel)
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

        __fb_sfx->voices[i].paused = 0;
    }

    SFX_DEBUG("sfx_sfx_resume: channel=%d", channel);
}


/* ------------------------------------------------------------------------- */
/* Resume all SFX                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxResumeAll()

    Resume every paused SFX voice.
*/

void fb_sfxSfxResumeAll(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; i++)
    {
        if (__fb_sfx->voices[i].active &&
            __fb_sfx->voices[i].type == FB_SFX_VOICE_SFX)
            __fb_sfx->voices[i].paused = 0;
    }

    SFX_DEBUG("sfx_sfx_resume: all");
}


/* end of sfx_sfx_resume.c */
