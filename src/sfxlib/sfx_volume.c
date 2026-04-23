/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_volume.c

    Purpose:

        Implement the BASIC VOLUME command.

        Volume controls the amplitude scaling applied to
        voices routed through a channel.

    Responsibilities:

        • manage global volume state
        • control per-channel volume
        • provide safe volume clamping

    This file intentionally does NOT contain:

        • mixer algorithms
        • oscillator generation
        • driver interaction
        • command parsing

    Architectural overview:

        VOLUME command
              │
              ▼
        channel volume state
              │
              ▼
        mixer scaling
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Volume limits                                                             */
/* ------------------------------------------------------------------------- */

#define FB_SFX_VOLUME_MIN 0.0f
#define FB_SFX_VOLUME_MAX 1.0f
#define FB_SFX_VOLUME_DEFAULT 1.0f


/* ------------------------------------------------------------------------- */
/* Global volume                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxVolume()

    Set the master volume for the audio system.

    Parameters:

        level   amplitude scaling (0.0 – 1.0)
*/

void fb_sfxVolume(float level)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (level < FB_SFX_VOLUME_MIN)
        level = FB_SFX_VOLUME_MIN;

    if (level > FB_SFX_VOLUME_MAX)
        level = FB_SFX_VOLUME_MAX;

    fb_sfxRuntimeLock();
    __fb_sfx->master_volume = level;
    fb_sfxRuntimeUnlock();
    SFX_DEBUG("sfx_volume: master volume set to %f", level);
}


/* ------------------------------------------------------------------------- */
/* Get master volume                                                         */
/* ------------------------------------------------------------------------- */

float fb_sfxVolumeGet(void)
{
    float level;

    if (!__fb_sfx)
        return FB_SFX_VOLUME_DEFAULT;

    fb_sfxRuntimeLock();
    level = __fb_sfx->master_volume;
    fb_sfxRuntimeUnlock();

    return level;
}


/* ------------------------------------------------------------------------- */
/* Channel volume                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxVolumeChannel()

    Set volume for a specific channel.

    Parameters:

        channel   channel index
        level     amplitude scaling (0.0 – 1.0)
*/

void fb_sfxVolumeChannel(int channel, float level)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    if (level < FB_SFX_VOLUME_MIN)
        level = FB_SFX_VOLUME_MIN;

    if (level > FB_SFX_VOLUME_MAX)
        level = FB_SFX_VOLUME_MAX;

    fb_sfxChannelSetVolume(channel, level);

    SFX_DEBUG(
        "sfx_volume: channel %d volume set to %f",
        channel,
        level
    );
}


/* ------------------------------------------------------------------------- */
/* Get channel volume                                                        */
/* ------------------------------------------------------------------------- */

float fb_sfxVolumeChannelGet(int channel)
{
    if (!__fb_sfx)
        return FB_SFX_VOLUME_DEFAULT;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return FB_SFX_VOLUME_DEFAULT;

    return fb_sfxChannelGetVolume(channel);
}


/* ------------------------------------------------------------------------- */
/* Reset volume                                                              */
/* ------------------------------------------------------------------------- */

void fb_sfxVolumeReset(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();
    __fb_sfx->master_volume = FB_SFX_VOLUME_DEFAULT;

    for (i = 0; i < FB_SFX_MAX_CHANNELS; i++)
        fb_sfxChannelSetVolume(i, FB_SFX_VOLUME_DEFAULT);
    fb_sfxRuntimeUnlock();

    SFX_DEBUG("sfx_volume: volume reset");
}


/* end of sfx_volume.c */
