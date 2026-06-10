/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_solaris.c

    Purpose:

        Provide Solaris/illumos backend state and worker-thread plumbing
        for sfxlib.

    Responsibilities:

        - own the Solaris/illumos backend state
        - register the Solaris/illumos audio driver before the null driver
        - provide the shared background-feed lifecycle used by the driver

    This file intentionally does NOT contain:

        - audio device ioctl handling
        - mixer implementation
        - BASIC-facing command parsing
*/

#include "fb_sfx_solaris.h"

#define FB_SFX_BSD_LABEL "Solaris"
#define FB_SFX_BSD_STATE_NAME fb_sfx_solaris
#define FB_SFX_BSD_DRIVER_NAME fb_sfxDriverSolarisAudio
#define FB_SFX_BSD_DEBUG_ENV "SFXLIB_SOLARIS_DEBUG"
#define FB_SFX_BSD_INIT_FN fb_sfxSolarisInit
#define FB_SFX_BSD_EXIT_FN fb_sfxSolarisExit
#define FB_SFX_BSD_WRITE_FN fb_sfxSolarisWrite
#define FB_SFX_BSD_RUNNING_FN fb_sfxSolarisRunning
#define FB_SFX_BSD_ACTIVATE_FN fb_sfxSolarisActivate
#define FB_SFX_BSD_DEACTIVATE_FN fb_sfxSolarisDeactivate
#define FB_SFX_BSD_ENABLE_WORKER 0

#include "../unix/sfx_bsd_template.inc"

/* end of sfx_solaris.c */
