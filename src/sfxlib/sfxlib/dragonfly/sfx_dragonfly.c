/*
    Preliminary DragonFly backend scaffold.
*/

#ifndef DISABLE_DRAGONFLY

#include "fb_sfx_dragonfly.h"

#define FB_SFX_BSD_LABEL "DRAGONFLY"
#define FB_SFX_BSD_STATE_NAME fb_sfx_dragonfly
#define FB_SFX_BSD_DRIVER_NAME fb_sfxDriverDragonflyOss
#define FB_SFX_BSD_DEBUG_ENV "SFXLIB_DRAGONFLY_DEBUG"
#define FB_SFX_BSD_INIT_FN fb_sfxDragonflyInit
#define FB_SFX_BSD_EXIT_FN fb_sfxDragonflyExit
#define FB_SFX_BSD_WRITE_FN fb_sfxDragonflyWrite
#define FB_SFX_BSD_RUNNING_FN fb_sfxDragonflyRunning
#define FB_SFX_BSD_ACTIVATE_FN fb_sfxDragonflyActivate
#define FB_SFX_BSD_DEACTIVATE_FN fb_sfxDragonflyDeactivate

#include "../unix/sfx_bsd_template.inc"

#endif

/* end of sfx_dragonfly.c */
