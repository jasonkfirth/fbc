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

} FB_SFX_DRIVER_STATS;

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
