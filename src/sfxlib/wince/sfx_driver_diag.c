/*
    FreeBASIC Sound Library support for Windows CE
    -----------------------------------------------

    File: sfx_driver_diag.c

    Purpose:

        Maintain bounded audio-driver counters on Windows CE without relying
        on the desktop environment-variable or stream-dump facilities.

    Responsibilities:

        - maintain lightweight per-driver counters
        - keep driver diagnostics silent on the device display

    This file intentionally does NOT contain:

        - audio mixing logic
        - sample dumping or environment-variable controls
        - WinMM API calls
        - driver selection logic
*/

#include <string.h>

#include "../fb_sfx_driver_diag.h"
#include "../fb_sfx_internal.h"

#define FB_SFX_DRIVER_STATS_SLOTS 16

typedef struct FB_SFX_DRIVER_STATS_SLOT
{
    int used;
    const char *driver_name;
    FB_SFX_DRIVER_STATS stats;

} FB_SFX_DRIVER_STATS_SLOT;

static FB_SFX_DRIVER_STATS_SLOT g_driver_stats[FB_SFX_DRIVER_STATS_SLOTS];

/* ------------------------------------------------------------------------- */
/* Driver counter helpers                                                    */
/* ------------------------------------------------------------------------- */

/*
    Counter ownership

    The sound core records write results after platform calls return.  That
    keeps requested, accepted, short-write, and drop accounting consistent for
    all drivers.  Platform drivers can add backend-specific events such as
    underrun recovery when they have that information.

    Counter locking

    The public counter helpers take the runtime lock themselves.  Some call
    sites already hold it, and the runtime lock is recursive on threaded
    builds, but keeping the rule here prevents future driver-specific
    diagnostics from accidentally racing DEVICE INFO snapshots.
*/

static int fb_sfxDriverStatsNameEquals(const char *a, const char *b)
{
    if (!a || !b)
        return 0;

    return strcmp(a, b) == 0;
}

static FB_SFX_DRIVER_STATS_SLOT *fb_sfxDriverStatsFind(const char *driver_name,
                                                       int create)
{
    int i;
    int free_index;

    if (!driver_name || !*driver_name)
        return NULL;

    free_index = -1;

    for (i = 0; i < FB_SFX_DRIVER_STATS_SLOTS; ++i)
    {
        if (g_driver_stats[i].used)
        {
            if (fb_sfxDriverStatsNameEquals(g_driver_stats[i].driver_name,
                                            driver_name))
            {
                return &g_driver_stats[i];
            }
        }
        else if (free_index < 0)
        {
            free_index = i;
        }
    }

    if (!create || free_index < 0)
        return NULL;

    g_driver_stats[free_index].used = 1;
    g_driver_stats[free_index].driver_name = driver_name;
    memset(&g_driver_stats[free_index].stats,
           0,
           sizeof(g_driver_stats[free_index].stats));

    return &g_driver_stats[free_index];
}

void fb_sfxDriverStatsReset(const char *driver_name)
{
    FB_SFX_DRIVER_STATS_SLOT *slot;

    fb_sfxRuntimeLock();

    slot = fb_sfxDriverStatsFind(driver_name, 1);
    if (slot)
        memset(&slot->stats, 0, sizeof(slot->stats));

    fb_sfxRuntimeUnlock();
}

void fb_sfxDriverStatsRecordWrite(const char *driver_name,
                                  int frames_requested,
                                  int write_result)
{
    FB_SFX_DRIVER_STATS_SLOT *slot;
    FB_SFX_DRIVER_STATS *stats;
    int accepted;

    if (frames_requested <= 0)
        return;

    fb_sfxRuntimeLock();

    slot = fb_sfxDriverStatsFind(driver_name, 1);
    if (!slot)
    {
        fb_sfxRuntimeUnlock();
        return;
    }

    stats = &slot->stats;
    stats->write_calls++;
    stats->frames_requested += (unsigned long long)frames_requested;

    if (write_result < 0)
    {
        stats->errors++;
        stats->last_error = write_result;
        fb_sfxRuntimeUnlock();
        return;
    }

    accepted = write_result;
    if (accepted > frames_requested)
        accepted = frames_requested;

    stats->frames_accepted += (unsigned long long)accepted;

    if (accepted == 0)
        stats->zero_writes++;

    if (accepted < frames_requested)
        stats->short_writes++;

    fb_sfxRuntimeUnlock();
}

void fb_sfxDriverStatsRecordDrop(const char *driver_name, int frames)
{
    FB_SFX_DRIVER_STATS_SLOT *slot;

    if (frames <= 0)
        return;

    fb_sfxRuntimeLock();

    slot = fb_sfxDriverStatsFind(driver_name, 1);
    if (slot)
        slot->stats.frames_dropped += (unsigned long long)frames;

    fb_sfxRuntimeUnlock();
}

void fb_sfxDriverStatsRecordUnderrun(const char *driver_name)
{
    FB_SFX_DRIVER_STATS_SLOT *slot;

    fb_sfxRuntimeLock();

    slot = fb_sfxDriverStatsFind(driver_name, 1);
    if (slot)
        slot->stats.underruns++;

    fb_sfxRuntimeUnlock();
}

void fb_sfxDriverStatsRecordOverrun(const char *driver_name)
{
    FB_SFX_DRIVER_STATS_SLOT *slot;

    fb_sfxRuntimeLock();

    slot = fb_sfxDriverStatsFind(driver_name, 1);
    if (slot)
        slot->stats.overruns++;

    fb_sfxRuntimeUnlock();
}

void fb_sfxDriverStatsRecordRecovery(const char *driver_name)
{
    FB_SFX_DRIVER_STATS_SLOT *slot;

    fb_sfxRuntimeLock();

    slot = fb_sfxDriverStatsFind(driver_name, 1);
    if (slot)
        slot->stats.recoveries++;

    fb_sfxRuntimeUnlock();
}

void fb_sfxDriverStatsRecordQueueFill(const char *driver_name, int frames)
{
    FB_SFX_DRIVER_STATS_SLOT *slot;

    if (frames < 0)
        return;

    fb_sfxRuntimeLock();

    slot = fb_sfxDriverStatsFind(driver_name, 1);
    if (slot)
    {
        slot->stats.current_queue_fill = frames;
        if (frames > slot->stats.max_queue_fill)
            slot->stats.max_queue_fill = frames;
    }

    fb_sfxRuntimeUnlock();
}

int fb_sfxDriverStatsSnapshot(const char *driver_name,
                              FB_SFX_DRIVER_STATS *stats)
{
    FB_SFX_DRIVER_STATS_SLOT *slot;
    int result;

    if (!stats)
        return -1;

    memset(stats, 0, sizeof(*stats));

    fb_sfxRuntimeLock();

    result = -1;
    slot = fb_sfxDriverStatsFind(driver_name, 0);
    if (slot)
    {
        *stats = slot->stats;
        result = 0;
    }

    fb_sfxRuntimeUnlock();
    return result;
}

void fb_sfxDriverStatsLog(const char *driver_name, const char *prefix)
{
    FB_SFX_DRIVER_STATS stats;
    const char *tag;

    if (fb_sfxDriverStatsSnapshot(driver_name, &stats) != 0)
        return;

    tag = (prefix && *prefix) ? prefix : "sfx_driver_diag";

    SFX_DEBUG("%s: stats driver=%s writes=%llu requested=%llu accepted=%llu dropped=%llu short=%llu zero=%llu errors=%llu underruns=%llu overruns=%llu recoveries=%llu queue=%d max_queue=%d last_error=%d",
              tag,
              driver_name ? driver_name : "(null)",
              stats.write_calls,
              stats.frames_requested,
              stats.frames_accepted,
              stats.frames_dropped,
              stats.short_writes,
              stats.zero_writes,
              stats.errors,
              stats.underruns,
              stats.overruns,
              stats.recoveries,
              stats.current_queue_fill,
              stats.max_queue_fill,
              stats.last_error);
}

/* ------------------------------------------------------------------------- */
/* Driver diagnostics                                                        */
/* ------------------------------------------------------------------------- */

void fb_sfxDriverDiagnostics(const char *driver_name,
                             const float *buffer,
                             int frames,
                             int channels)
{
    (void)driver_name;
    (void)buffer;
    (void)frames;
    (void)channels;
}

/* end of sfx_driver_diag.c */
