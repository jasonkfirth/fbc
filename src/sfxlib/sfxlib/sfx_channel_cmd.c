/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_channel_cmd.c

    Purpose:

        Implement the BASIC CHANNEL command.

        CHANNEL selects the active audio channel used by
        subsequent sound commands.

    Responsibilities:

        • manage the current command routing channel
        • enforce channel index limits
        • provide helpers to query/reset channel state

    This file intentionally does NOT contain:

        • mixer logic
        • oscillator generation
        • driver interaction
        • voice allocation

    Architectural overview:

        CHANNEL command
              │
              ▼
        command routing state
              │
              ▼
        NOTE / SOUND / PLAY
              │
              ▼
        voice generation
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Set current channel                                                       */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxChannelCmd()

    Select the active channel for subsequent sound commands.

    Parameters:

        channel   channel index
*/

void fb_sfxChannelCmd(int channel)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0)
        channel = 0;

    if (channel >= FB_SFX_MAX_CHANNELS)
        channel = FB_SFX_MAX_CHANNELS - 1;

    __fb_sfx->current_channel = channel;

    SFX_DEBUG(
        "sfx_channel_cmd: current channel set to %d",
        channel
    );
}


/* ------------------------------------------------------------------------- */
/* Get current channel                                                       */
/* ------------------------------------------------------------------------- */

int fb_sfxChannelCmdGet(void)
{
    if (!__fb_sfx)
        return 0;

    return __fb_sfx->current_channel;
}


/* ------------------------------------------------------------------------- */
/* Reset current channel                                                     */
/* ------------------------------------------------------------------------- */

void fb_sfxChannelCmdReset(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    __fb_sfx->current_channel = 0;

    SFX_DEBUG(
        "sfx_channel_cmd: current channel reset to 0"
    );
}


/* ------------------------------------------------------------------------- */
/* Resolve channel helper                                                    */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxResolveChannel()

    Helper used by command modules.

    If the caller supplies a negative channel value,
    the current channel context will be used instead.
*/

int fb_sfxResolveChannel(int channel)
{
    if (!__fb_sfx)
        return 0;

    if (channel < 0)
        return __fb_sfx->current_channel;

    if (channel >= FB_SFX_MAX_CHANNELS)
        return FB_SFX_MAX_CHANNELS - 1;

    return channel;
}


/* end of sfx_channel_cmd.c */
