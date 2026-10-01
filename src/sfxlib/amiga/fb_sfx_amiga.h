/*
    FreeBASIC Sound Library support for AmigaOS
    ----------------------------------------

    File: fb_sfx_amiga.h

    Purpose:

        Declare services shared by the native AmigaOS AHI backend.

    Responsibilities:

        - expose background mixer worker lifecycle operations
        - keep AMIGA-only declarations out of portable sfxlib headers

    This file intentionally does NOT contain:

        - AHI device state
        - PCM conversion
        - mixer implementation
        - public FreeBASIC sound APIs
*/

#ifndef FB_SFX_AMIGA_H
#define FB_SFX_AMIGA_H

int fb_sfxAmigaWorkerStart(int buffer_frames);
void fb_sfxAmigaWorkerStop(void);

#endif

/* end of fb_sfx_amiga.h */
