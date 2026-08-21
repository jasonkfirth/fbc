/*
    FreeBASIC Sound Library support for Windows CE
    -----------------------------------------------

    File: sfx_platform.c

    Purpose:

        Provide the Windows CE target's shared sfxlib teardown hook.

    Responsibilities:

        - preserve the platform lifecycle contract used by the shared core
        - leave driver-owned cleanup to the WinMM driver shutdown callback

    This file intentionally does NOT contain:

        - WinMM device state
        - MIDI worker state
        - shared mixer teardown
*/

void fb_sfxPlatformExit(void)
{
}

/* end of sfx_platform.c */
