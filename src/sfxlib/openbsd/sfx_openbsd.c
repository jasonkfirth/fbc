/*
    Preliminary OpenBSD backend scaffold.
*/

#ifndef DISABLE_OPENBSD

#include "fb_sfx_openbsd.h"

#define FB_SFX_BSD_LABEL "OPENBSD"
#define FB_SFX_BSD_STATE_NAME fb_sfx_openbsd
#define FB_SFX_BSD_DRIVER_NAME fb_sfxDriverOpenbsdOss
#define FB_SFX_BSD_DEBUG_ENV "SFXLIB_OPENBSD_DEBUG"
#define FB_SFX_BSD_INIT_FN fb_sfxOpenbsdInit
#define FB_SFX_BSD_EXIT_FN fb_sfxOpenbsdExit
#define FB_SFX_BSD_WRITE_FN fb_sfxOpenbsdWrite
#define FB_SFX_BSD_RUNNING_FN fb_sfxOpenbsdRunning
#define FB_SFX_BSD_ACTIVATE_FN fb_sfxOpenbsdActivate
#define FB_SFX_BSD_DEACTIVATE_FN fb_sfxOpenbsdDeactivate

#include "../unix/sfx_bsd_template.inc"

#endif

/* end of sfx_openbsd.c */
