/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_driver_diag.h

    Purpose:

        Declare optional diagnostics used by platform audio drivers.

    Responsibilities:

        - expose driver-edge sample dump helpers
        - keep diagnostic declarations out of individual driver files

    This file intentionally does NOT contain:

        - audio mixing logic
        - platform audio API calls
        - driver selection logic
*/

#ifndef FB_SFX_DRIVER_DIAG_H
#define FB_SFX_DRIVER_DIAG_H

void fb_sfxDriverDiagnostics(const char *driver_name,
                             const float *buffer,
                             int frames,
                             int channels);

#endif

/* end of fb_sfx_driver_diag.h */
