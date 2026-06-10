/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_sfx_load.c

    Purpose:

        Implement the SFX LOAD command.

        This loads a sound effect file into memory so it
        can later be triggered by SFX PLAY.

    Responsibilities:

        • load sound effect files from disk
        • allocate memory for SFX data
        • register SFX assets in the internal table
        • safely unload sound effects

    This file intentionally does NOT contain:

        • audio decoding
        • mixer interaction
        • driver interaction
        • playback logic

    Architectural overview:

        SFX LOAD
             │
             ▼
        sound effect asset table
             │
             ▼
        SFX PLAY
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Internal helpers                                                          */
/* ------------------------------------------------------------------------- */

static void fb_sfxSfxStopIdLocked(int id)
{
    int i;

    if (!__fb_sfx)
        return;

    for (i = 0; i < FB_SFX_MAX_VOICES; ++i)
    {
        FB_SFXVOICE *voice = &__fb_sfx->voices[i];

        if (voice->active &&
            voice->type == FB_SFX_VOICE_SFX &&
            voice->sfx_id == id)
        {
            voice->active = 0;
            voice->data = NULL;
        }
    }
}


/* ------------------------------------------------------------------------- */
/* SFX LOAD                                                                  */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxLoad()

    Load a sound effect file.

    Parameters:

        id        sound effect identifier
        filename  path to file
*/

void fb_sfxSfxLoad(int id, const char *filename)
{
    FB_SFX_ASSET next_asset;
    float *old_data;
    float *decoded = NULL;
    float *mono = NULL;
    int frames = 0;
    int channels = 0;
    int sample_rate = 0;
    int frame;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_SFX)
        return;

    if (!filename || !*filename)
        return;

    if (fb_sfxDecodeFile(filename,
                         &decoded,
                         &frames,
                         &channels,
                         &sample_rate) != 0)
        return;

    if (frames <= 0 || channels <= 0)
    {
        free(decoded);
        return;
    }

    mono = (float *)malloc((size_t)frames * sizeof(float));
    if (!mono)
    {
        free(decoded);
        return;
    }

    for (frame = 0; frame < frames; ++frame)
    {
        int ch;
        float sample = 0.0f;

        for (ch = 0; ch < channels; ++ch)
            sample += decoded[(frame * channels) + ch];

        mono[frame] = sample / (float)channels;
    }

    free(decoded);

    memset(&next_asset, 0, sizeof(next_asset));

    next_asset.data = mono;
    next_asset.size = frames * (int)sizeof(float);
    next_asset.frames = frames;
    next_asset.sample_rate = sample_rate;
    next_asset.loaded = 1;

    strncpy(next_asset.name, filename, sizeof(next_asset.name) - 1);
    next_asset.name[sizeof(next_asset.name) - 1] = '\0';

    fb_sfxRuntimeLock();

    old_data = __fb_sfx->sfx[id].data;
    fb_sfxSfxStopIdLocked(id);
    __fb_sfx->sfx[id] = next_asset;

    fb_sfxRuntimeUnlock();

    free(old_data);

    SFX_DEBUG(
        "sfx_sfx_load: id=%d '%s' frames=%d rate=%d",
        id,
        filename,
        frames,
        sample_rate
    );
}


/* ------------------------------------------------------------------------- */
/* SFX UNLOAD                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxSfxUnload()

    Remove a sound effect from memory.
*/

void fb_sfxSfxUnload(int id)
{
    float *old_data;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_SFX)
        return;

    fb_sfxRuntimeLock();

    old_data = __fb_sfx->sfx[id].data;
    fb_sfxSfxStopIdLocked(id);

    __fb_sfx->sfx[id].data = NULL;
    __fb_sfx->sfx[id].size = 0;
    __fb_sfx->sfx[id].frames = 0;
    __fb_sfx->sfx[id].sample_rate = 0;
    __fb_sfx->sfx[id].loaded = 0;
    __fb_sfx->sfx[id].name[0] = '\0';

    fb_sfxRuntimeUnlock();

    free(old_data);

    SFX_DEBUG("sfx_sfx_load: unloaded id=%d", id);
}


/* end of sfx_sfx_load.c */
