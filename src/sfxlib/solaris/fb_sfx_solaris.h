/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_solaris.h

    Purpose:

        Declare the Solaris/illumos audio backend state and lifecycle
        helpers used by sfxlib.

    Responsibilities:

        - expose backend state shared by the worker scaffold and driver
        - declare backend initialization and teardown hooks
        - keep Solaris/illumos declarations out of generic sfxlib headers

    This file intentionally does NOT contain:

        - mixer internals
        - device ioctl handling
        - FreeBASIC command implementations
*/

#ifndef __FB_SFX_SOLARIS_H__
#define __FB_SFX_SOLARIS_H__

#include "../unix/fb_sfx_bsd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern FB_SFX_BSD_STATE fb_sfx_solaris;

int  fb_sfxSolarisInit(void);
void fb_sfxSolarisExit(void);
int  fb_sfxSolarisWrite(float *buffer, int frames);
int  fb_sfxSolarisRunning(void);
int  fb_sfxSolarisActivate(int rate, int channels, int buffer_frames);
void fb_sfxSolarisDeactivate(void);

#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_solaris.h */
