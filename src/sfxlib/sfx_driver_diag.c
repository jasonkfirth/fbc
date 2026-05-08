/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_driver_diag.c

    Purpose:

        Implement optional diagnostics for samples handed to platform
        audio drivers.

    Responsibilities:

        - dump the stream seen at the driver write boundary
        - limit diagnostic output so unattended example runs cannot
          create unbounded files

    This file intentionally does NOT contain:

        - audio mixing logic
        - platform audio API calls
        - driver selection logic

    Diagnostic stream:

        SFXLIB_DRIVER_DUMP writes one mono floating point sample per
        line. Stereo buffers are averaged so the file can be analyzed
        with the same fastfft.bas helper used for mixer dumps.
*/

#include <stdio.h>
#include <stdlib.h>

#include "fb_sfx_driver_diag.h"

/* ------------------------------------------------------------------------- */
/* Environment helpers                                                       */
/* ------------------------------------------------------------------------- */

static FILE *fb_sfxDriverDumpFile(void)
{
    static int initialized = 0;
    static FILE *file = NULL;

    if (!initialized)
    {
        const char *path = getenv("SFXLIB_DRIVER_DUMP");

        initialized = 1;

        if (path && *path)
            file = fopen(path, "w");
    }

    return file;
}

static int fb_sfxDriverDumpFrameLimit(void)
{
    static int initialized = 0;
    static int limit = 44100;

    if (!initialized)
    {
        const char *value = getenv("SFXLIB_DRIVER_DUMP_FRAMES");

        initialized = 1;

        if (value && *value)
        {
            int parsed = atoi(value);

            if (parsed > 0)
                limit = parsed;
        }
    }

    return limit;
}

/* ------------------------------------------------------------------------- */
/* Driver diagnostics                                                        */
/* ------------------------------------------------------------------------- */

void fb_sfxDriverDiagnostics(const char *driver_name,
                             const float *buffer,
                             int frames,
                             int channels)
{
    static int dumped_frames = 0;
    FILE *dump;
    int limit;
    int frame;

    (void)driver_name;

    if (!buffer || frames <= 0)
        return;

    if (channels <= 0)
        channels = 1;

    dump = fb_sfxDriverDumpFile();
    if (!dump)
        return;

    limit = fb_sfxDriverDumpFrameLimit();
    if (dumped_frames >= limit)
        return;

    for (frame = 0; frame < frames && dumped_frames < limit; frame++, dumped_frames++)
    {
        double mixed = 0.0;
        int channel;

        for (channel = 0; channel < channels; channel++)
            mixed += buffer[(frame * channels) + channel];

        mixed /= (double)channels;
        fprintf(dump, "%.9g\n", mixed);
    }

    fflush(dump);
}

/* end of sfx_driver_diag.c */
