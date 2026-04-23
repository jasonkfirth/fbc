/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_control.c

    Purpose:

        Implement runtime control functions for the FreeBASIC sound
        subsystem.

        These functions provide the operational interface used by
        higher-level commands to control the behavior of the audio
        engine.

    Responsibilities:

        • device enumeration
        • device selection
        • channel configuration
        • master volume control
        • runtime status queries

    This file intentionally does NOT contain:

        • audio synthesis algorithms
        • mixer implementation
        • platform driver code
        • audio buffer management

    Architectural overview:

        BASIC command
              │
              ▼
        sfx_control.c
              │
              ▼
        runtime state / driver interface
*/

#include <stdio.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"
#include "fb_sfx_driver.h"
#include "fb_sfx_mixer.h"


/* ------------------------------------------------------------------------- */
/* Device enumeration                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceList()

    Request a list of audio devices from the active driver.

    Returns:

        number of devices detected
*/

int fb_sfxControlDeviceList(void)
{
    if (!fb_sfxEnsureInitialized())
        return 0;

    if (__fb_sfx->driver && __fb_sfx->driver->device_list)
        return __fb_sfx->driver->device_list();

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Device selection                                                          */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDeviceSelect()

    Select an audio device by index.

    Returns:

        0 on success
        non-zero on failure
*/

int fb_sfxControlDeviceSelect(int device)
{
    if (!fb_sfxEnsureInitialized())
        return -1;

    if (__fb_sfx->driver && __fb_sfx->driver->device_select)
        return __fb_sfx->driver->device_select(device);

    return -1;
}


/* ------------------------------------------------------------------------- */
/* Master volume                                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSetMasterVolume()

    Set master output volume for the sound subsystem.

    Volume range:

        0.0  = silent
        1.0  = full volume
*/

void fb_sfxSetMasterVolume(float volume)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (volume < 0.0f)
        volume = 0.0f;

    if (volume > 1.0f)
        volume = 1.0f;

    __fb_sfx->master_volume = volume;

    SFX_DEBUG("sfx_control: master volume set to %f", volume);
}


/*
    fb_sfxGetMasterVolume()

    Return current master volume level.
*/

float fb_sfxGetMasterVolume(void)
{
    if (!fb_sfxEnsureInitialized())
        return 0.0f;

    return __fb_sfx->master_volume;
}


/* ------------------------------------------------------------------------- */
/* Channel control                                                           */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSetChannelVolume()

    Set volume for a specific audio channel.
*/

void fb_sfxSetChannelVolume(int channel, float volume)
{
    FB_SFXCHANNEL *ch;

    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    ch = &__fb_sfx->channels[channel];

    if (volume < 0.0f)
        volume = 0.0f;

    if (volume > 1.0f)
        volume = 1.0f;

    ch->volume = volume;

    SFX_DEBUG("sfx_control: channel %d volume = %f", channel, volume);
}


/*
    fb_sfxSetChannelPan()

    Set stereo pan position for a channel.

    Range:

        -1.0 = left
         0.0 = center
         1.0 = right
*/

void fb_sfxSetChannelPan(int channel, float pan)
{
    FB_SFXCHANNEL *ch;

    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    ch = &__fb_sfx->channels[channel];

    if (pan < -1.0f)
        pan = -1.0f;

    if (pan > 1.0f)
        pan = 1.0f;

    ch->pan = pan;

    SFX_DEBUG("sfx_control: channel %d pan = %f", channel, pan);
}


/* ------------------------------------------------------------------------- */
/* Runtime status                                                            */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxStatus()

    Return basic status of the sound subsystem.

    Returns:

        0  subsystem not initialized
        1  subsystem active
*/

int fb_sfxStatus(void)
{
    if (__fb_sfx == NULL)
        return 0;

    return __fb_sfx->initialized ? 1 : 0;
}


/* ------------------------------------------------------------------------- */
/* Driver information                                                        */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDriverName()

    Return the name of the currently active audio driver.
*/

const char *fb_sfxDriverName(void)
{
    if (!fb_sfxEnsureInitialized())
        return "none";

    if (!__fb_sfx->driver)
        return "none";

    return __fb_sfx->driver->name;
}


/* end of sfx_control.c */
