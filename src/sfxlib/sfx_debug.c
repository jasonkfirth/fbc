#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "fb_sfx_internal.h"

static int g_sfx_debug_initialized = 0;
static int g_sfx_debug_enabled = 0;

void fb_sfxDebugInit(void)
{
    const char *env;

    if (g_sfx_debug_initialized)
        return;

    g_sfx_debug_initialized = 1;

    env = getenv("SFXLIB_DEBUG");
    g_sfx_debug_enabled = (env && *env && *env != '0');
}

int fb_sfxDebugEnabled(void)
{
    fb_sfxDebugInit();
    return g_sfx_debug_enabled;
}

void fb_sfxDebugLog(const char *fmt, ...)
{
    va_list ap;

    if (!fb_sfxDebugEnabled())
        return;

    fputs("SFX: ", stderr);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
}
