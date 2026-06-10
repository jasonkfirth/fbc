/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_load.c

    Purpose:

        Implement the MUSIC LOAD command.

        This loads one music file into memory and makes it the current
        music asset. Loading another file replaces the previous asset.

    Responsibilities:

        • load music files from disk
        • allocate memory for music data
        • store current music asset metadata
        • provide safe loading and unloading

    This file intentionally does NOT contain:

        • music playback
        • streaming logic
        • driver interaction
        • decoding of specific music formats

    Architectural overview:

        MUSIC LOAD
             │
             ▼
        current music asset
             │
             ▼
        playback system
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* MUSIC LOAD                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxMusicLoad()

    Load a music file into memory.

    Parameters:

        filename   path to music file

    Returns:

        0 on success or -1 on failure
*/

int fb_sfxMusicLoad(const char *filename)
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
        return -1;

    if (!filename || !*filename)
        return -1;

    if (fb_sfxDecodeFile(filename,
                         &decoded,
                         &frames,
                         &channels,
                         &sample_rate) != 0)
        return -1;

    if (frames <= 0 || channels <= 0)
    {
        free(decoded);
        return -1;
    }

    mono = (float *)malloc((size_t)frames * sizeof(float));
    if (!mono)
    {
        free(decoded);
        return -1;
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

    old_data = __fb_sfx->music.data;
    fb_sfxMusicStopLocked();
    __fb_sfx->music = next_asset;

    fb_sfxRuntimeUnlock();

    free(old_data);

    SFX_DEBUG(
        "sfx_music_load: loaded '%s' frames=%d rate=%d",
        filename,
        frames,
        sample_rate
    );

    return 0;
}


/* ------------------------------------------------------------------------- */
/* MUSIC UNLOAD                                                              */
/* ------------------------------------------------------------------------- */

void fb_sfxMusicUnload(void)
{
    float *old_data;

    if (!fb_sfxEnsureInitialized())
        return;

    fb_sfxRuntimeLock();

    old_data = __fb_sfx->music.data;
    fb_sfxMusicStopLocked();

    __fb_sfx->music.data = NULL;
    __fb_sfx->music.size = 0;
    __fb_sfx->music.frames = 0;
    __fb_sfx->music.sample_rate = 0;
    __fb_sfx->music.loaded = 0;
    __fb_sfx->music.name[0] = '\0';

    fb_sfxRuntimeUnlock();

    free(old_data);

    SFX_DEBUG("sfx_music_load: unloaded current music");
}


/* end of sfx_music_load.c */
