/*
    FreeBASIC sfxlib tests
    ----------------------

    File: midi-fm-test-rate.c

    Purpose:

        Let the software MIDI test exercise its sample-rate-dependent
        synthesis without requiring a physical audio device at every rate.

    Responsibilities:

        - validate the requested test sample rate
        - update the initialized sfxlib context under its runtime lock

    This file intentionally does NOT contain:

        - production sample-rate selection
        - audio driver configuration
        - synthesizer implementation

    The null driver consumes a caller-specified number of frames, so this
    helper only changes the rate used by sample-rate-dependent synthesis.
    It is linked into the test executable and is not part of sfxlib.
*/

#include "../../src/sfxlib/fb_sfx.h"
#include "../../src/sfxlib/fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Test-only sample-rate control                                             */
/* ------------------------------------------------------------------------- */

int fb_sfxTestSetSampleRate(int sample_rate)
{
    int result;

    if (sample_rate < 8000 || sample_rate > 192000)
        return -1;

    if (fb_sfxEnsureInit() != 0)
        return -1;

    result = -1;

    fb_sfxRuntimeLock();

    if (__fb_sfx && __fb_sfx->initialized)
    {
        __fb_sfx->samplerate = sample_rate;
        result = 0;
    }

    fb_sfxRuntimeUnlock();

    return result;
}

/* end of midi-fm-test-rate.c */
