/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_wave_cmd.c

    Purpose:

        Implement the BASIC WAVE command.

        The command defines which waveform generator should
        be used for a waveform identifier.

    Responsibilities:

        • define waveform types
        • validate waveform identifiers
        • provide command-layer access to waveform definitions

    This file intentionally does NOT contain:

        • oscillator logic
        • waveform sample generation
        • mixer logic
        • driver interaction

    Architectural overview:

        WAVE command
             │
             ▼
        waveform definition table
             │
             ▼
        oscillator system
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Supported waveform types                                                  */
/* ------------------------------------------------------------------------- */

#define FB_SFX_WAVE_SINE      0
#define FB_SFX_WAVE_SQUARE    1
#define FB_SFX_WAVE_TRIANGLE  2
#define FB_SFX_WAVE_SAW       3
#define FB_SFX_WAVE_NOISE     4


/* ------------------------------------------------------------------------- */
/* WAVE command                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxWaveCmd()

    Define a waveform type.

    Parameters:

        id             waveform identifier
        waveform_type  generator type
*/

void fb_sfxWaveCmd(int id, int waveform_type)
{
    FB_SFX_WAVE *wave;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_WAVES)
        return;

    /* validate waveform type */

    if (waveform_type < FB_SFX_WAVE_SINE)
        waveform_type = FB_SFX_WAVE_SINE;

    if (waveform_type > FB_SFX_WAVE_NOISE)
        waveform_type = FB_SFX_WAVE_NOISE;

    wave = &__fb_sfx->waves[id];

    wave->type = waveform_type;
    wave->defined = 1;

    SFX_DEBUG(
        "sfx_wave_cmd: id=%d type=%d",
        id,
        waveform_type
    );
}


/* ------------------------------------------------------------------------- */
/* Query waveform definition                                                 */
/* ------------------------------------------------------------------------- */

int fb_sfxWaveDefined(int id)
{
    if (!__fb_sfx)
        return 0;

    if (id < 0 || id >= FB_SFX_MAX_WAVES)
        return 0;

    return __fb_sfx->waves[id].defined;
}


/* ------------------------------------------------------------------------- */
/* Reset waveform definitions                                                */
/* ------------------------------------------------------------------------- */

void fb_sfxWaveCmdReset(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    for (i = 0; i < FB_SFX_MAX_WAVES; i++)
        __fb_sfx->waves[i].defined = 0;

    SFX_DEBUG("sfx_wave_cmd: waveform table reset");
}


/* end of sfx_wave_cmd.c */
