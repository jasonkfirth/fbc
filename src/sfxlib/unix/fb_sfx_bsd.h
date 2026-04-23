/*
    Shared declarations for the BSD backend scaffolding.
*/

#ifndef __FB_SFX_BSD_H__
#define __FB_SFX_BSD_H__

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
