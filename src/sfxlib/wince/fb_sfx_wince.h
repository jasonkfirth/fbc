/*
    FreeBASIC Sound Library support for Windows CE
    -----------------------------------------------

    File: fb_sfx_wince.h

    Purpose:

        Declare the private driver registry contract for the Windows CE
        sfxlib backend.

    Responsibilities:

        - expose the WinMM waveform output driver to the platform registry
        - keep Windows CE driver declarations in the target replacement tree

    This file intentionally does NOT contain:

        - mixer state
        - waveform driver implementation
        - MIDI synthesis or transport logic
*/

#ifndef FB_SFX_WINCE_H
#define FB_SFX_WINCE_H

#include "../fb_sfx_driver.h"

extern const FB_SFX_DRIVER fb_sfxDriverWinMM;

#endif

/* end of fb_sfx_wince.h */
