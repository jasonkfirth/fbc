/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_tempo.c

    Purpose:

        Implement the BASIC TEMPO command.

        Tempo defines the musical playback speed in beats per minute
        and affects note durations used by PLAY strings, sequencers,
        and other music systems.

    Responsibilities:

        • manage global tempo state
        • provide tempo access functions
        • enforce safe tempo limits

    This file intentionally does NOT contain:

        • audio synthesis
        • mixer logic
        • driver interaction
        • music parsing

    Architectural overview:

        TEMPO command
              │
              ▼
        tempo state
              │
              ▼
        music scheduling systems
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Tempo limits                                                              */
/* ------------------------------------------------------------------------- */

#define FB_SFX_TEMPO_MIN 20
#define FB_SFX_TEMPO_MAX 400
#define FB_SFX_TEMPO_DEFAULT 120


/* ------------------------------------------------------------------------- */
/* Set tempo                                                                 */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxTempo()

    Set the global music tempo.

    Parameters:

        bpm   beats per minute
*/

void fb_sfxTempo(int bpm)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (bpm < FB_SFX_TEMPO_MIN)
        bpm = FB_SFX_TEMPO_MIN;

    if (bpm > FB_SFX_TEMPO_MAX)
        bpm = FB_SFX_TEMPO_MAX;

    __fb_sfx->tempo = bpm;

    SFX_DEBUG("sfx_tempo: tempo set to %d BPM", bpm);
}


/* ------------------------------------------------------------------------- */
/* Get tempo                                                                 */
/* ------------------------------------------------------------------------- */

int fb_sfxTempoGet(void)
{
    if (!__fb_sfx)
        return FB_SFX_TEMPO_DEFAULT;

    return __fb_sfx->tempo;
}


/* ------------------------------------------------------------------------- */
/* Reset tempo                                                               */
/* ------------------------------------------------------------------------- */

void fb_sfxTempoReset(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    __fb_sfx->tempo = FB_SFX_TEMPO_DEFAULT;

    SFX_DEBUG("sfx_tempo: tempo reset to %d BPM",
              FB_SFX_TEMPO_DEFAULT);
}


/* ------------------------------------------------------------------------- */
/* Beat duration helper                                                      */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxTempoBeatDuration()

    Return the duration of one beat in seconds.

    This helper is used by music scheduling systems
    when converting musical note lengths into
    real-time durations.
*/

float fb_sfxTempoBeatDuration(void)
{
    int tempo;

    if (!__fb_sfx)
        return 0.5f;

    tempo = __fb_sfx->tempo;

    if (tempo <= 0)
        tempo = FB_SFX_TEMPO_DEFAULT;

    return 60.0f / (float)tempo;
}


/* end of sfx_tempo.c */
