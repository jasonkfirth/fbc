/*
    FreeBASIC Sound Library support for AROS
    ----------------------------------------

    File: fb_sfx_aros.h

    Purpose:

        Declare services shared by the native AROS AHI backend.

    Responsibilities:

        - expose background mixer worker lifecycle operations
        - keep AROS-only declarations out of portable sfxlib headers

    This file intentionally does NOT contain:

        - AHI device state
        - PCM conversion
        - mixer implementation
        - public FreeBASIC sound APIs
*/

#ifndef FB_SFX_AROS_H
#define FB_SFX_AROS_H

int fb_sfxArosWorkerStart(int buffer_frames);
void fb_sfxArosWorkerStop(void);

#endif

/* end of fb_sfx_aros.h */
