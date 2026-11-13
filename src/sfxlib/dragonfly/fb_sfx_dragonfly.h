/*
 * Project: FreeBASIC runtime libraries
 * File: fb_sfx_dragonfly.h
 * Purpose: Share the dragonfly sound backend interface.
 * Responsibilities: Backend state and playback lifecycle declarations.
 * This file intentionally does NOT contain: Device I/O or audio mixing.
 */
/*
    Shared declarations for the DragonFly backend layer.
*/

#ifndef FB_SFX_DRAGONFLY_H
#define FB_SFX_DRAGONFLY_H

#include "../unix/fb_sfx_bsd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern FB_SFX_BSD_STATE fb_sfx_dragonfly;

int fb_sfxDragonflyInit(void);
void fb_sfxDragonflyExit(void);
int fb_sfxDragonflyActivate(int rate, int channels, int buffer_frames);
void fb_sfxDragonflyDeactivate(void);
int fb_sfxDragonflyWrite(float *buffer, int frames);
int fb_sfxDragonflyRunning(void);

#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_dragonfly.h */
