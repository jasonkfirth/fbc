/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_music_cmd.c

    Purpose:

        Provide small command-facing helpers for the MUSIC family.

        MUSIC LOAD keeps one current music file. The compiler surface
        also allows direct file-oriented forms such as MUSIC PLAY
        "song.wav". These helpers bridge that small semantic gap.

    Responsibilities:

        • map file-oriented MUSIC commands onto current-music helpers
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

int fb_sfxMusicPlayCmd(void)
{
    return fb_sfxMusicPlay();
}

int fb_sfxMusicLoopCmd(void)
{
    return fb_sfxMusicLoop();
}

int fb_sfxMusicPlayFile(const char *filename)
{
    int result;

    result = fb_sfxMusicLoad(filename);
    if (result != 0)
        return result;

    return fb_sfxMusicPlay();
}

int fb_sfxMusicLoopFile(const char *filename)
{
    int result;

    result = fb_sfxMusicLoad(filename);
    if (result != 0)
        return result;

    return fb_sfxMusicLoop();
}

/* end of sfx_music_cmd.c */
