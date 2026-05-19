/*
    DragonFly OSS sound driver.
*/

#ifndef DISABLE_DRAGONFLY

#include "fb_sfx_dragonfly.h"

#define FB_SFX_BSD_LABEL "DRAGONFLY"
#define FB_SFX_BSD_DRIVER_NAME fb_sfxDriverDragonflyOss
#define FB_SFX_BSD_INIT_FN fb_sfxDragonflyInit
#define FB_SFX_BSD_EXIT_FN fb_sfxDragonflyExit
#define FB_SFX_BSD_ACTIVATE_FN fb_sfxDragonflyActivate
#define FB_SFX_BSD_DEACTIVATE_FN fb_sfxDragonflyDeactivate
#define FB_SFX_BSD_RUNNING_FN fb_sfxDragonflyRunning

#include "../unix/sfx_driver_oss_template.inc"

#endif

/* end of sfx_driver_oss.c */
