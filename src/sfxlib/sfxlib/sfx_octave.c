/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_octave.c

    Purpose:

        Implement the BASIC OCTAVE command.

        The octave value determines the default octave used when
        interpreting musical note commands such as NOTE and PLAY.

    Responsibilities:

        • manage global octave state
        • enforce valid octave limits
        • provide helper functions for music systems

    This file intentionally does NOT contain:

        • oscillator generation
        • mixer logic
        • driver interaction
        • command parsing

    Architectural overview:

        OCTAVE command
              │
              ▼
        octave state
              │
              ▼
        NOTE / PLAY parsing
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Octave limits                                                             */
/* ------------------------------------------------------------------------- */

#define FB_SFX_OCTAVE_MIN     0
#define FB_SFX_OCTAVE_MAX     8
#define FB_SFX_OCTAVE_DEFAULT 4


/* ------------------------------------------------------------------------- */
/* Set octave                                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxOctave()

    Set the default musical octave used by note commands.

    Parameters:

        octave  octave index (0–8)
*/

void fb_sfxOctave(int octave)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (octave < FB_SFX_OCTAVE_MIN)
        octave = FB_SFX_OCTAVE_MIN;

    if (octave > FB_SFX_OCTAVE_MAX)
        octave = FB_SFX_OCTAVE_MAX;

    __fb_sfx->octave = octave;

    SFX_DEBUG("sfx_octave: octave set to %d", octave);
}


/* ------------------------------------------------------------------------- */
/* Get octave                                                                */
/* ------------------------------------------------------------------------- */

int fb_sfxOctaveGet(void)
{
    if (!__fb_sfx)
        return FB_SFX_OCTAVE_DEFAULT;

    return __fb_sfx->octave;
}


/* ------------------------------------------------------------------------- */
/* Reset octave                                                              */
/* ------------------------------------------------------------------------- */

void fb_sfxOctaveReset(void)
{
    if (!fb_sfxEnsureInitialized())
        return;

    __fb_sfx->octave = FB_SFX_OCTAVE_DEFAULT;

    SFX_DEBUG(
        "sfx_octave: octave reset to %d",
        FB_SFX_OCTAVE_DEFAULT
    );
}


/* ------------------------------------------------------------------------- */
/* Octave shift helpers                                                      */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxOctaveUp()

    Increase the current octave by one.
*/

void fb_sfxOctaveUp(void)
{
    int octave;

    if (!fb_sfxEnsureInitialized())
        return;

    octave = __fb_sfx->octave + 1;

    if (octave > FB_SFX_OCTAVE_MAX)
        octave = FB_SFX_OCTAVE_MAX;

    __fb_sfx->octave = octave;

    SFX_DEBUG("sfx_octave: octave increased to %d", octave);
}


/*
    fb_sfxOctaveDown()

    Decrease the current octave by one.
*/

void fb_sfxOctaveDown(void)
{
    int octave;

    if (!fb_sfxEnsureInitialized())
        return;

    octave = __fb_sfx->octave - 1;

    if (octave < FB_SFX_OCTAVE_MIN)
        octave = FB_SFX_OCTAVE_MIN;

    __fb_sfx->octave = octave;

    SFX_DEBUG("sfx_octave: octave decreased to %d", octave);
}


/* end of sfx_octave.c */
