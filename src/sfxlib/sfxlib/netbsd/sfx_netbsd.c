/*
    Preliminary NetBSD backend scaffold.
*/

#ifndef DISABLE_NETBSD

#include "fb_sfx_netbsd.h"

#define FB_SFX_BSD_LABEL "NETBSD"
#define FB_SFX_BSD_STATE_NAME fb_sfx_netbsd
#define FB_SFX_BSD_DRIVER_NAME fb_sfxDriverNetbsdOss
#define FB_SFX_BSD_DEBUG_ENV "SFXLIB_NETBSD_DEBUG"
#define FB_SFX_BSD_INIT_FN fb_sfxNetbsdInit
#define FB_SFX_BSD_EXIT_FN fb_sfxNetbsdExit
#define FB_SFX_BSD_WRITE_FN fb_sfxNetbsdWrite
#define FB_SFX_BSD_RUNNING_FN fb_sfxNetbsdRunning
#define FB_SFX_BSD_ACTIVATE_FN fb_sfxNetbsdActivate
#define FB_SFX_BSD_DEACTIVATE_FN fb_sfxNetbsdDeactivate

#include "../unix/sfx_bsd_template.inc"

#endif

/* end of sfx_netbsd.c */
