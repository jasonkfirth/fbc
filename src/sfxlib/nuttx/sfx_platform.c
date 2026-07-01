/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_platform.c

    Purpose:

        Provide the NuttX target's shared sfxlib teardown hook.

    Responsibilities:

        - give the common sfxlib runtime a platform exit symbol
        - keep NuttX audio backend ownership inside sfxlib/nuttx

    This file intentionally does NOT contain:

        - mixer logic
        - hardware audio output
        - MIDI support
*/

void fb_sfxPlatformExit(void)
{
}

/* end of sfx_platform.c */
