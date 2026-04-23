/*
    Shared preliminary BSD OSS driver scaffold.

    This placeholder intentionally fails initialization today. It exists
    to give each BSD target a clear target-local driver slot and a known
    place to wire real OSS/audio(4)/sndio work later.
*/

#include "../fb_sfx.h"
#include "../fb_sfx_driver.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef FB_SFX_BSD_LABEL
#error "FB_SFX_BSD_LABEL must be defined before including sfx_driver_oss_template.inc"
#endif

#ifndef FB_SFX_BSD_DRIVER_NAME
#error "FB_SFX_BSD_DRIVER_NAME must be defined before including sfx_driver_oss_template.inc"
#endif

static int bsd_oss_debug_enabled(void)
{
    const char *env = getenv("SFXLIB_DEBUG");
    return (env && *env && *env != '0');
}

#define BSD_OSS_DBG(...) \
    do { if (bsd_oss_debug_enabled()) fprintf(stderr, "SFX_%s_OSS: " __VA_ARGS__, FB_SFX_BSD_LABEL); } while (0)

static int bsd_oss_init(int rate, int channels, int buffer_frames, int flags)
{
    (void)rate;
    (void)channels;
    (void)buffer_frames;
    (void)flags;

    BSD_OSS_DBG("placeholder backend not yet implemented\n");
    return -1;
}

static void bsd_oss_exit(void)
{
}

static int bsd_oss_write(const float *buffer, int frames)
{
    (void)buffer;
    (void)frames;
    return -1;
}

const FB_SFX_DRIVER FB_SFX_BSD_DRIVER_NAME =
{
    FB_SFX_BSD_LABEL " OSS",
    0,
    bsd_oss_init,
    bsd_oss_exit,
    bsd_oss_write,
    NULL,
    NULL,
    NULL,
    NULL
};

#undef BSD_OSS_DBG
