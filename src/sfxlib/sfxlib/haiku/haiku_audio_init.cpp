#ifndef DISABLE_HAIKU

#include "fb_sfx_haiku.h"

#include <MediaRoster.h>
#include <MediaNode.h>

#include <stdio.h>
#include <stdlib.h>

static BMediaRoster *g_media_roster = NULL;
static media_node    g_audio_output;

static int g_audio_debug_initialized = 0;
static int g_audio_debug_enabled = 0;

static void fb_hHaikuAudioInitDebug(void)
{
    const char *env;

    if (g_audio_debug_initialized)
        return;

    g_audio_debug_initialized = 1;

    env = getenv("HAIKU_SFX_AUDIO_DEBUG");
    g_audio_debug_enabled = (env && *env && *env != '0');
}

static int fb_hHaikuAudioDebugEnabled(void)
{
    fb_hHaikuAudioInitDebug();
    return g_audio_debug_enabled;
}

/* SAFE macro — no __VA_ARGS__ */
#define HAIKU_AUDIO_DBG(msg) \
    do { \
        if (fb_hHaikuAudioDebugEnabled()) \
            fprintf(stderr, "HAIKU_AUDIO: %s", msg); \
    } while (0)

int fb_sfxHaikuInit(void)
{
    status_t err;

    if (g_media_roster)
        return 0;

    HAIKU_AUDIO_DBG("initializing Media Kit\n");

    g_media_roster = BMediaRoster::Roster();
    if (!g_media_roster)
    {
        HAIKU_AUDIO_DBG("failed to obtain BMediaRoster\n");
        return -1;
    }

    err = g_media_roster->GetAudioOutput(&g_audio_output);
    if (err != B_OK)
    {
        HAIKU_AUDIO_DBG("GetAudioOutput failed\n");
        g_media_roster = NULL;
        return -1;
    }

    return 0;
}

void fb_sfxHaikuExit(void)
{
    if (!g_media_roster)
        return;

    HAIKU_AUDIO_DBG("shutting down Media Kit\n");

    if (g_audio_output.node)
        g_media_roster->ReleaseNode(g_audio_output);

    g_audio_output = media_node();
    g_media_roster = NULL;
}

#endif
