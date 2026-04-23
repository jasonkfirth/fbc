/*
    Preliminary FreeBSD backend scaffold.
*/

#ifndef DISABLE_FREEBSD

#include "fb_sfx_freebsd.h"

#define FB_SFX_BSD_LABEL "FREEBSD"
#define FB_SFX_BSD_STATE_NAME fb_sfx_freebsd
#define FB_SFX_BSD_DRIVER_NAME fb_sfxDriverFreebsdOss
#define FB_SFX_BSD_DEBUG_ENV "SFXLIB_FREEBSD_DEBUG"
#define FB_SFX_BSD_INIT_FN fb_sfxFreebsdInit
#define FB_SFX_BSD_EXIT_FN fb_sfxFreebsdExit
#define FB_SFX_BSD_WRITE_FN fb_sfxFreebsdWrite
#define FB_SFX_BSD_RUNNING_FN fb_sfxFreebsdRunning
#define FB_SFX_BSD_ACTIVATE_FN fb_sfxFreebsdActivate
#define FB_SFX_BSD_DEACTIVATE_FN fb_sfxFreebsdDeactivate

#include "../unix/sfx_bsd_template.inc"

#endif

/* end of sfx_freebsd.c */
