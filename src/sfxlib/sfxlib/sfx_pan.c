/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_pan.c

    Purpose:

        Implement the BASIC PAN command.

        Panning controls the stereo position of audio generated
        on a specific channel.

    Responsibilities:

        • manage per-channel stereo panning
        • enforce valid pan ranges
        • provide helper functions for retrieving pan state

    This file intentionally does NOT contain:

        • mixer algorithms
        • oscillator generation
        • driver interaction
        • command parsing

    Architectural overview:

        PAN command
             │
             ▼
        channel pan state
             │
             ▼
        mixer stereo scaling
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Pan limits                                                                */
/* ------------------------------------------------------------------------- */

#define FB_SFX_PAN_LEFT   -1.0f
#define FB_SFX_PAN_CENTER  0.0f
#define FB_SFX_PAN_RIGHT   1.0f


/* ------------------------------------------------------------------------- */
/* Set pan                                                                   */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxPan()

    Set stereo pan position for a channel.

    Parameters:

        channel   channel index
        position  stereo position (-1.0 = left, 0 = center, 1.0 = right)
*/

void fb_sfxPan(int channel, float position)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    if (position < FB_SFX_PAN_LEFT)
        position = FB_SFX_PAN_LEFT;

    if (position > FB_SFX_PAN_RIGHT)
        position = FB_SFX_PAN_RIGHT;

    fb_sfxChannelSetPan(channel, position);

    SFX_DEBUG(
        "sfx_pan: channel %d pan set to %f",
        channel,
        position
    );
}


/* ------------------------------------------------------------------------- */
/* Get pan                                                                   */
/* ------------------------------------------------------------------------- */

float fb_sfxPanGet(int channel)
{
    if (!__fb_sfx)
        return FB_SFX_PAN_CENTER;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return FB_SFX_PAN_CENTER;

    return fb_sfxChannelGetPan(channel);
}


/* ------------------------------------------------------------------------- */
/* Reset pan                                                                 */
/* ------------------------------------------------------------------------- */

void fb_sfxPanReset(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    for (i = 0; i < FB_SFX_MAX_CHANNELS; i++)
        fb_sfxChannelSetPan(i, FB_SFX_PAN_CENTER);

    SFX_DEBUG("sfx_pan: all channels reset to center");
}


/* end of sfx_pan.c */
