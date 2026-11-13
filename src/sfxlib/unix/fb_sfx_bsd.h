/*
 * Project: FreeBASIC runtime libraries
 * File: fb_sfx_bsd.h
 * Purpose: Share the BSD sound backend state.
 * Responsibilities: Common playback state used by the platform adapters.
 * This file intentionally does NOT contain: Device I/O or audio mixing.
 */
/*
    Shared declarations for the BSD backend scaffolding.
*/

#ifndef FB_SFX_BSD_H
#define FB_SFX_BSD_H

#include "../fb_sfx_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FB_SFX_BSD_STATE
{
    int initialized;
    int sample_rate;
    int channels;
    int buffer_frames;
    int running;
} FB_SFX_BSD_STATE;

#ifdef __cplusplus
}
#endif

#endif

/* end of fb_sfx_bsd.h */
