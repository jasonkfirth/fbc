/*
    Shared declarations for the macOS backend layer.
*/

#ifndef __FB_SFX_DARWIN_H__
#define __FB_SFX_DARWIN_H__

#ifndef DISABLE_DARWIN

#include "../fb_sfx_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FB_SFX_DARWIN_STATE
{
    int initialized;
    int sample_rate;
    int channels;
    int buffer_frames;
    int running;
    void *device_handle;
} FB_SFX_DARWIN_STATE;

extern FB_SFX_DARWIN_STATE fb_sfx_darwin;

int fb_sfxDarwinInit(void);
void fb_sfxDarwinExit(void);
int fb_sfxDarwinWrite(float *buffer, int frames);
int fb_sfxDarwinRunning(void);
int fb_sfxDarwinActivate(int rate, int channels, int buffer_frames);
void fb_sfxDarwinDeactivate(void);

#ifdef __cplusplus
}
#endif

#endif
#endif

/* end of fb_sfx_darwin.h */
