/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_channel.c

    Purpose:

        Implement channel management for the FreeBASIC sound subsystem.

        Channels represent logical sound lanes used by BASIC commands
        to control groups of voices. Channel parameters affect all
        voices assigned to that channel.

    Responsibilities:

        • channel initialization
        • channel state management
        • channel volume and pan control
        • channel muting
        • instrument assignment

    This file intentionally does NOT contain:

        • voice allocation
        • audio mixing logic
        • platform driver interaction
        • BASIC command parsing

    Architectural overview:

        BASIC commands
             │
             ▼
        channel state
             │
             ▼
        voice system
             │
             ▼
        mixer
*/

#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Channel initialization                                                    */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxChannelInit()

    Reset all channels to their default state.
*/

void fb_sfxChannelInit(void)
{
    int i;

    if (!__fb_sfx)
        return;

    fb_sfxRuntimeLock();
    for (i = 0; i < FB_SFX_MAX_CHANNELS; i++)
    {
        FB_SFXCHANNEL *ch = &__fb_sfx->channels[i];

        memset(ch, 0, sizeof(FB_SFXCHANNEL));

        ch->volume = 1.0f;
        ch->pan = 0.0f;
        ch->mute = 0;
        ch->instrument = -1;
    }
    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_channel: initialized %d channels", FB_SFX_MAX_CHANNELS);
}


/* ------------------------------------------------------------------------- */
/* Channel validation                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxChannelValid()

    Verify that a channel index is valid.
*/

static int fb_sfxChannelValid(int channel)
{
    if (!__fb_sfx)
        return 0;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return 0;

    return 1;
}


/* ------------------------------------------------------------------------- */
/* Channel volume                                                            */
/* ------------------------------------------------------------------------- */

void fb_sfxChannelSetVolume(int channel, float volume)
{
    FB_SFXCHANNEL *ch;

    if (!fb_sfxChannelValid(channel))
        return;

    if (volume < 0.0f)
        volume = 0.0f;

    if (volume > 1.0f)
        volume = 1.0f;

    fb_sfxRuntimeLock();
    ch = &__fb_sfx->channels[channel];

    ch->volume = volume;
    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_channel: channel %d volume = %f", channel, volume);
}


float fb_sfxChannelGetVolume(int channel)
{
    float volume;

    if (!fb_sfxChannelValid(channel))
        return 0.0f;

    fb_sfxRuntimeLock();
    volume = __fb_sfx->channels[channel].volume;
    fb_sfxRuntimeUnlock();

    return volume;
}


/* ------------------------------------------------------------------------- */
/* Channel pan                                                               */
/* ------------------------------------------------------------------------- */

void fb_sfxChannelSetPan(int channel, float pan)
{
    FB_SFXCHANNEL *ch;

    if (!fb_sfxChannelValid(channel))
        return;

    if (pan < -1.0f)
        pan = -1.0f;

    if (pan > 1.0f)
        pan = 1.0f;

    fb_sfxRuntimeLock();
    ch = &__fb_sfx->channels[channel];

    ch->pan = pan;
    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_channel: channel %d pan = %f", channel, pan);
}


float fb_sfxChannelGetPan(int channel)
{
    float pan;

    if (!fb_sfxChannelValid(channel))
        return 0.0f;

    fb_sfxRuntimeLock();
    pan = __fb_sfx->channels[channel].pan;
    fb_sfxRuntimeUnlock();

    return pan;
}


/* ------------------------------------------------------------------------- */
/* Channel mute                                                              */
/* ------------------------------------------------------------------------- */

void fb_sfxChannelMute(int channel, int state)
{
    if (!fb_sfxChannelValid(channel))
        return;

    fb_sfxRuntimeLock();
    __fb_sfx->channels[channel].mute = state ? 1 : 0;
    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_channel: channel %d mute = %d", channel, state);
}


int fb_sfxChannelMuted(int channel)
{
    int muted;

    if (!fb_sfxChannelValid(channel))
        return 0;

    fb_sfxRuntimeLock();
    muted = __fb_sfx->channels[channel].mute;
    fb_sfxRuntimeUnlock();

    return muted;
}


/* ------------------------------------------------------------------------- */
/* Channel instrument                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxChannelSetInstrument()

    Assign an instrument definition to a channel.

    Instrument definitions will be implemented in the synthesis
    subsystem and referenced here by identifier.
*/

void fb_sfxChannelSetInstrument(int channel, int instrument)
{
    if (!fb_sfxChannelValid(channel))
        return;

    fb_sfxRuntimeLock();
    __fb_sfx->channels[channel].instrument = instrument;
    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_channel: channel %d instrument = %d",
              channel,
              instrument);
}


int fb_sfxChannelGetInstrument(int channel)
{
    int instrument;

    if (!fb_sfxChannelValid(channel))
        return -1;

    fb_sfxRuntimeLock();
    instrument = __fb_sfx->channels[channel].instrument;
    fb_sfxRuntimeUnlock();

    return instrument;
}


/* ------------------------------------------------------------------------- */
/* Channel reset                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxChannelReset()

    Reset a single channel to its default state.
*/

void fb_sfxChannelReset(int channel)
{
    FB_SFXCHANNEL *ch;

    if (!fb_sfxChannelValid(channel))
        return;

    fb_sfxRuntimeLock();
    ch = &__fb_sfx->channels[channel];

    memset(ch, 0, sizeof(FB_SFXCHANNEL));

    ch->volume = 1.0f;
    ch->pan = 0.0f;
    ch->instrument = -1;
    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_channel: channel %d reset", channel);
}


/*
    fb_sfxChannelResetAll()

    Reset every channel.
*/

void fb_sfxChannelResetAll(void)
{
    fb_sfxChannelInit();

    SFX_DEBUG("sfx_channel: all channels reset");
}


/* end of sfx_channel.c */
