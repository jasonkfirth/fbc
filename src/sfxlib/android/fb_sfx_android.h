#ifndef FB_SFX_ANDROID_H
#define FB_SFX_ANDROID_H

#include "../fb_sfx_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const FB_SFX_DRIVER fb_sfxDriverAAudio;
extern const FB_SFX_DRIVER fb_sfxDriverOpenSLES;

void fb_hAndroidSfxSetLifecycle(int started, int resumed);
int fb_hAndroidSfxIsRunning(void);

#ifdef __cplusplus
}
#endif

#endif
