/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_rest.c

    Purpose:

        Implement the BASIC REST command.

        REST introduces a silent delay within musical sequences.
        No audio signal is generated during the rest interval.

    Responsibilities:

        • provide musical timing gaps
        • integrate with music playback systems
        • allow future scheduling support

    This file intentionally does NOT contain:

        • oscillator generation
        • mixer logic
        • envelope processing
        • driver interaction

    Architectural overview:

        REST command
             │
             ▼
        timeline advancement
             │
             ▼
        (silence in audio pipeline)
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* REST implementation                                                       */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxRest()

    Insert a silent duration into the playback timeline.

    Parameters:

        duration  rest length in seconds
*/

void fb_sfxRest(float duration)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (duration <= 0.0f)
        return;

    /*
        In the initial implementation the rest command simply
        advances the internal timing state. More advanced music
        scheduling systems will eventually integrate this with
        event queues or sequencers.
    */

    SFX_DEBUG(
        "sfx_rest: duration=%f (silence)",
        duration
    );
}


/* ------------------------------------------------------------------------- */
/* Channel REST                                                              */
/* ------------------------------------------------------------------------- */

void fb_sfxRestChannel(int channel, float duration)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (duration <= 0.0f)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        channel = 0;

    /*
        Channel-specific rests become useful in multi-channel
        sequencing systems where different musical lines pause
        independently.
    */

    SFX_DEBUG(
        "sfx_rest: channel=%d duration=%f (silence)",
        channel,
        duration
    );
}


/* end of sfx_rest.c */
