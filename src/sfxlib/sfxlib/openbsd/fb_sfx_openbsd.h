/*
    Shared declarations for the OpenBSD backend layer.
*/

#ifndef __FB_SFX_OPENBSD_H__
#define __FB_SFX_OPENBSD_H__

#include "../unix/fb_sfx_bsd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern FB_SFX_BSD_STATE fb_sfx_openbsd;

int fb_sfxOpenbsdInit(void);
void fb_sfxOpenbsdExit(void);
int fb_sfxOpenbsdActivate(int rate, int channels, int buffer_frames);
void fb_sfxOpenbsdDeactivate(void);
int fb_sfxOpenbsdWrite(float *buffer, int frames);
int fb_sfxOpenbsdRunning(void);

#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_openbsd.h */
