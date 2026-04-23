/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_load.c

    Purpose:

        Implement the MUSIC LOAD command.

        This loads a music file into memory and registers it
        with the sfxlib music subsystem so it can be played later.

    Responsibilities:

        • load music files from disk
        • allocate memory for music data
        • store music asset metadata
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
        music asset table
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

        music identifier or -1 on failure
*/

int fb_sfxMusicLoad(const char *filename)
{
    float *decoded = NULL;
    float *mono = NULL;
    int frames = 0;
    int channels = 0;
    int sample_rate = 0;
    int id;
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

    /* find free music slot */

    for (id = 0; id < FB_SFX_MAX_MUSIC; id++)
    {
        if (!__fb_sfx->music[id].loaded)
            break;
    }

    if (id >= FB_SFX_MAX_MUSIC)
    {
        free(mono);
        return -1;
    }

    __fb_sfx->music[id].data = (unsigned char *)mono;
    __fb_sfx->music[id].size = frames * (int)sizeof(float);
    __fb_sfx->music[id].loaded = 1;

    strncpy(
        __fb_sfx->music[id].name,
        filename,
        sizeof(__fb_sfx->music[id].name) - 1
    );

    __fb_sfx->music[id].name[sizeof(__fb_sfx->music[id].name) - 1] = '\0';

    SFX_DEBUG(
        "sfx_music_load: loaded '%s' id=%d frames=%d rate=%d",
        filename,
        id,
        frames,
        sample_rate
    );

    return id;
}


/* ------------------------------------------------------------------------- */
/* MUSIC UNLOAD                                                              */
/* ------------------------------------------------------------------------- */

void fb_sfxMusicUnload(int id)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_MUSIC)
        return;

    if (!__fb_sfx->music[id].loaded)
        return;

    free(__fb_sfx->music[id].data);

    __fb_sfx->music[id].data = NULL;
    __fb_sfx->music[id].size = 0;
    __fb_sfx->music[id].loaded = 0;

    SFX_DEBUG("sfx_music_load: unloaded id=%d", id);
}


/* end of sfx_music_load.c */
