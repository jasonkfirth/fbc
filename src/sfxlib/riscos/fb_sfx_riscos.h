/*
    FreeBASIC Sound Library support for RISC OS
    -------------------------------------------

    File: fb_sfx_riscos.h

    Purpose:

        Declare the shared services owned by the RISC OS sfxlib backend.

    Responsibilities:

        - expose audio-worker lifecycle operations to the device driver
        - keep RISC OS backend declarations out of portable sfxlib headers

    This file intentionally does NOT contain:

        - audio device state
        - PCM conversion
        - mixer implementation
        - public FreeBASIC sound APIs
*/

#ifndef FB_SFX_RISCOS_H
#define FB_SFX_RISCOS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Background mixer worker                                                   */
/* ------------------------------------------------------------------------- */

int fb_sfxRiscosWorkerStart(int buffer_frames);
void fb_sfxRiscosWorkerStop(void);

#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_riscos.h */
