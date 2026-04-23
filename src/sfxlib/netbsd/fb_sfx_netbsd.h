/*
    Shared declarations for the NetBSD backend layer.
*/

#ifndef __FB_SFX_NETBSD_H__
#define __FB_SFX_NETBSD_H__

#include "../unix/fb_sfx_bsd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern FB_SFX_BSD_STATE fb_sfx_netbsd;

int fb_sfxNetbsdInit(void);
void fb_sfxNetbsdExit(void);
int fb_sfxNetbsdActivate(int rate, int channels, int buffer_frames);
void fb_sfxNetbsdDeactivate(void);
int fb_sfxNetbsdWrite(float *buffer, int frames);
int fb_sfxNetbsdRunning(void);

#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_netbsd.h */
