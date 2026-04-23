/*
    Shared declarations for the FreeBSD backend layer.
*/

#ifndef __FB_SFX_FREEBSD_H__
#define __FB_SFX_FREEBSD_H__

#include "../unix/fb_sfx_bsd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern FB_SFX_BSD_STATE fb_sfx_freebsd;

int fb_sfxFreebsdInit(void);
void fb_sfxFreebsdExit(void);
int fb_sfxFreebsdActivate(int rate, int channels, int buffer_frames);
void fb_sfxFreebsdDeactivate(void);
int fb_sfxFreebsdWrite(float *buffer, int frames);
int fb_sfxFreebsdRunning(void);

#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_freebsd.h */
