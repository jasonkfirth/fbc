/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_haiku.h

    Purpose:

        Declare the shared Haiku backend state and helper entry points.

    Responsibilities:

        - expose Haiku playback state
        - declare Haiku playback lifecycle helpers
        - declare Haiku capture lifecycle helpers
        - keep platform declarations out of generic sfxlib headers

    This file intentionally does NOT contain:

        - playback implementation
        - capture implementation
        - mixer logic
        - BASIC command declarations
*/

#ifndef __FB_SFX_HAIKU_H__
#define __FB_SFX_HAIKU_H__

#ifndef DISABLE_HAIKU

#include "../fb_sfx_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FB_SFX_HAIKU_STATE
{
    int initialized;
    int sample_rate;
    int channels;
    int buffer_frames;
    int running;
} FB_SFX_HAIKU_STATE;

extern FB_SFX_HAIKU_STATE fb_sfx_haiku;

int  fb_sfxHaikuInit(void);
void fb_sfxHaikuExit(void);

int fb_sfxHaikuWrite(const float *buffer, int frames);

int  fb_sfxHaikuCaptureInit(void);
int  fb_sfxHaikuCaptureStart(void);
void fb_sfxHaikuCaptureStop(void);
int  fb_sfxHaikuCaptureRead(float *buffer, int frames);

int fb_sfxHaikuRunning(void);

void fb_sfxHaikuInitDebug(void);
int  fb_sfxHaikuDebugEnabled(void);

#ifdef __cplusplus
}
#endif

#endif
#endif

/* end of fb_sfx_haiku.h */
