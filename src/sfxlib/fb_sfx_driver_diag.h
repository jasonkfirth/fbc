/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx_driver_diag.h

    Purpose:

        Declare optional diagnostics used by platform audio drivers.

    Responsibilities:

        - expose driver-edge sample dump helpers
        - expose shared driver counter helpers
        - keep diagnostic declarations out of individual driver files

    This file intentionally does NOT contain:

        - audio mixing logic
        - platform audio API calls
        - driver selection logic
*/

#ifndef FB_SFX_DRIVER_DIAG_H
#define FB_SFX_DRIVER_DIAG_H

/* Natural ABI padding keeps the diagnostic snapshot aligned on each target. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#endif
typedef struct FB_SFX_DRIVER_STATS
{
    unsigned long long write_calls;
    unsigned long long frames_requested;
    unsigned long long frames_accepted;
    unsigned long long frames_dropped;
    unsigned long long underruns;
    unsigned long long overruns;
    unsigned long long short_writes;
    unsigned long long zero_writes;
    unsigned long long errors;
    unsigned long long recoveries;

    int current_queue_fill;
    int max_queue_fill;
    int last_error;
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
    /* Name the existing four-byte tail so the Win64 snapshot size stays fixed. */
    unsigned char reserved_tail_padding[4];
#endif

} FB_SFX_DRIVER_STATS;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

void fb_sfxDriverDiagnostics(const char *driver_name,
                             const float *buffer,
                             int frames,
                             int channels);

void fb_sfxDriverStatsReset(const char *driver_name);
void fb_sfxDriverStatsRecordWrite(const char *driver_name,
                                  int frames_requested,
                                  int write_result);
void fb_sfxDriverStatsRecordDrop(const char *driver_name, int frames);
void fb_sfxDriverStatsRecordUnderrun(const char *driver_name);
void fb_sfxDriverStatsRecordOverrun(const char *driver_name);
void fb_sfxDriverStatsRecordRecovery(const char *driver_name);
void fb_sfxDriverStatsRecordQueueFill(const char *driver_name, int frames);
int fb_sfxDriverStatsSnapshot(const char *driver_name,
                              FB_SFX_DRIVER_STATS *stats);
void fb_sfxDriverStatsLog(const char *driver_name, const char *prefix);

#endif

/* end of fb_sfx_driver_diag.h */
