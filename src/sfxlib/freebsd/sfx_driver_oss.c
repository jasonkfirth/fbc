/*
    FreeBSD OSS sound driver.
*/

#ifndef DISABLE_FREEBSD

#include "fb_sfx_freebsd.h"

#define FB_SFX_BSD_LABEL "FREEBSD"
#define FB_SFX_BSD_DRIVER_NAME fb_sfxDriverFreebsdOss
#define FB_SFX_BSD_INIT_FN fb_sfxFreebsdInit
#define FB_SFX_BSD_EXIT_FN fb_sfxFreebsdExit
#define FB_SFX_BSD_ACTIVATE_FN fb_sfxFreebsdActivate
#define FB_SFX_BSD_DEACTIVATE_FN fb_sfxFreebsdDeactivate
#define FB_SFX_BSD_RUNNING_FN fb_sfxFreebsdRunning

#include "../unix/sfx_driver_oss_template.inc"

#endif

/* end of sfx_driver_oss.c */
