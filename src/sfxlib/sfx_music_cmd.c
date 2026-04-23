/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_cmd.c

    Purpose:

        Provide small command-facing helpers for the MUSIC family.

        The backend already exposes asset-oriented routines such as
        MUSIC LOAD followed by MUSIC PLAY id.  The compiler surface
        also wants direct file-oriented forms such as MUSIC PLAY
        "song.wav".  These helpers bridge that small semantic gap.

    Responsibilities:

        • map file-oriented MUSIC commands onto asset-oriented helpers
        • preserve simple integer return values for command/function use

    This file intentionally does NOT contain:

        • music decoding
        • music mixer logic
        • file format parsing
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* MUSIC command helpers                                                     */
/* ------------------------------------------------------------------------- */

int fb_sfxMusicPlayCmd(int id)
{
    fb_sfxMusicPlay(id);
    return id;
}

int fb_sfxMusicLoopCmd(int id)
{
    fb_sfxMusicLoop(id);
    return id;
}

int fb_sfxMusicPlayFile(const char *filename)
{
    int id;

    id = fb_sfxMusicLoad(filename);
    if (id < 0)
        return id;

    fb_sfxMusicPlay(id);
    return id;
}

int fb_sfxMusicLoopFile(const char *filename)
{
    int id;

    id = fb_sfxMusicLoad(filename);
    if (id < 0)
        return id;

    fb_sfxMusicLoop(id);
    return id;
}

/* end of sfx_music_cmd.c */
