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

    /* free previous SFX if slot already used */

    if (__fb_sfx->sfx[id].loaded)
        free(__fb_sfx->sfx[id].data);

    __fb_sfx->sfx[id].data = (unsigned char *)mono;
    __fb_sfx->sfx[id].size = frames * (int)sizeof(float);
    __fb_sfx->sfx[id].loaded = 1;

    strncpy(
        __fb_sfx->sfx[id].name,
        filename,
        sizeof(__fb_sfx->sfx[id].name) - 1
    );

    __fb_sfx->sfx[id].name[
        sizeof(__fb_sfx->sfx[id].name) - 1
    ] = '\0';

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
    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_SFX)
        return;

    if (!__fb_sfx->sfx[id].loaded)
        return;

    free(__fb_sfx->sfx[id].data);

    __fb_sfx->sfx[id].data = NULL;
    __fb_sfx->sfx[id].size = 0;
    __fb_sfx->sfx[id].loaded = 0;

    SFX_DEBUG("sfx_sfx_load: unloaded id=%d", id);
}


/* end of sfx_sfx_load.c */
