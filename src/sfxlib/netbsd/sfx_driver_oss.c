/*
    Preliminary NetBSD OSS driver placeholder.
*/

#ifndef DISABLE_NETBSD

#include "fb_sfx_netbsd.h"

#define FB_SFX_BSD_LABEL "NETBSD"
#define FB_SFX_BSD_DRIVER_NAME fb_sfxDriverNetbsdOss

#include "../unix/sfx_driver_oss_template.inc"

#endif

/* end of sfx_driver_oss.c */
