/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_voice_cmd.c

    Purpose:

        Implement the BASIC VOICE command.

        VOICE selects the current instrument for the active
        command channel.

    Responsibilities:

        • assign an instrument to the current command channel
        • provide a helper to query the current voice selection

    This file intentionally does NOT contain:

        • instrument definition logic
        • mixer algorithms
        • oscillator generation
        • driver interaction

    Architectural overview:

        VOICE command
             |
             v
        current command channel
             |
             v
        channel instrument state
             |
             v
        SOUND / NOTE / PLAY / TONE
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* VOICE command                                                             */
/* ------------------------------------------------------------------------- */

void fb_sfxVoice(int instrument)
{
    int channel;

    if (!fb_sfxEnsureInitialized())
        return;

    channel = fb_sfxChannelCmdGet();

    if (instrument < 0)
    {
        fb_sfxChannelSetInstrument(channel, -1);
        SFX_DEBUG("sfx_voice_cmd: channel=%d instrument cleared", channel);
        return;
    }

    if (instrument >= FB_SFX_MAX_INSTRUMENTS)
        return;

    if (!fb_sfxInstrumentDefined(instrument))
        return;

    fb_sfxChannelSetInstrument(channel, instrument);

    SFX_DEBUG(
        "sfx_voice_cmd: channel=%d instrument=%d",
        channel,
        instrument
    );
}


/* ------------------------------------------------------------------------- */
/* Query current voice                                                       */
/* ------------------------------------------------------------------------- */

int fb_sfxVoiceGet(void)
{
    int channel;

    if (!__fb_sfx)
        return -1;

    channel = fb_sfxChannelCmdGet();

    return fb_sfxChannelGetInstrument(channel);
}


/* end of sfx_voice_cmd.c */
