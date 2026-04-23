/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_play_cmd.c

    Purpose:

        Provide command-facing control helpers for the PLAY family.

        PLAY already turns music strings into voice activity through
        sfx_play.c.  This file adds the missing control verbs so that
        PLAY can be stopped, paused, resumed, and queried without
        having to treat it as a different subsystem.

    Responsibilities:

        • stop active PLAY voices
        • pause and resume PLAY voices
        • report aggregate PLAY state

    This file intentionally does NOT contain:

        • MML parsing
        • tone generation
        • mixer logic
        • platform driver behavior
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* PLAY control helpers                                                      */
/* ------------------------------------------------------------------------- */

void fb_sfxPlayStop(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxVoiceStopType(FB_SFX_VOICE_PLAY);
}

void fb_sfxPlayPause(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxVoicePauseType(FB_SFX_VOICE_PLAY);
}

void fb_sfxPlayResume(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxVoiceResumeType(FB_SFX_VOICE_PLAY);
}

int fb_sfxPlayStatus(void)
{
    return fb_sfxVoiceStatusType(FB_SFX_VOICE_PLAY);
}

/* end of sfx_play_cmd.c */
