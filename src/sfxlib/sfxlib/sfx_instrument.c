/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_instrument.c

    Purpose:

        Manage instrument definitions used by the synthesis
        system.

        Instruments group together waveform and envelope
        parameters so they can be applied to voices easily.

    Responsibilities:

        • store instrument definitions
        • validate instrument identifiers
        • provide helpers to assign instruments to channels

    This file intentionally does NOT contain:

        • oscillator generation
        • envelope processing
        • mixer logic
        • command parsing

    Architectural overview:

        INSTRUMENT command
             │
             ▼
        instrument definition table
             │
             ▼
        voice allocation
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Define instrument                                                         */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxInstrumentDefine()

    Define a new instrument.

    Parameters:

        id          instrument identifier
        wave_id     waveform identifier
        env_id      envelope identifier
*/

void fb_sfxInstrumentDefine(int id, int wave_id, int env_id)
{
    FB_SFX_INSTRUMENT *inst;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_INSTRUMENTS)
        return;

    if (wave_id < 0 || wave_id >= FB_SFX_MAX_WAVES)
        wave_id = 0;

    if (env_id < 0 || env_id >= FB_SFX_MAX_ENVELOPES)
        env_id = -1;

    inst = &__fb_sfx->instruments[id];

    inst->wave_id = wave_id;
    inst->env_id  = env_id;
    inst->defined = 1;

    SFX_DEBUG(
        "sfx_instrument: id=%d wave=%d env=%d",
        id,
        wave_id,
        env_id
    );
}


/* ------------------------------------------------------------------------- */
/* Assign instrument to channel                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxInstrumentAssign()

    Assign an instrument to a channel.
*/

void fb_sfxInstrumentAssign(int channel, int instrument_id)
{
    if (!fb_sfxEnsureInitialized())
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    if (instrument_id < 0 || instrument_id >= FB_SFX_MAX_INSTRUMENTS)
        return;

    if (!__fb_sfx->instruments[instrument_id].defined)
        return;

    __fb_sfx->channels[channel].instrument = instrument_id;

    SFX_DEBUG(
        "sfx_instrument: channel=%d instrument=%d",
        channel,
        instrument_id
    );
}


/* ------------------------------------------------------------------------- */
/* Apply instrument to voice                                                 */
/* ------------------------------------------------------------------------- */

void fb_sfxInstrumentApply(
    FB_SFXVOICE *voice,
    int channel,
    int default_wave,
    int default_env
)
{
    int instrument_id;
    FB_SFX_INSTRUMENT *inst;

    if (!voice)
        return;

    fb_sfxVoiceSetWaveform(voice, default_wave);

    if (default_env >= 0)
        fb_sfxVoiceSetEnvelope(voice, default_env);

    if (!__fb_sfx)
        return;

    if (channel < 0 || channel >= FB_SFX_MAX_CHANNELS)
        return;

    instrument_id = __fb_sfx->channels[channel].instrument;

    if (instrument_id < 0 || instrument_id >= FB_SFX_MAX_INSTRUMENTS)
        return;

    inst = &__fb_sfx->instruments[instrument_id];

    if (!inst->defined)
        return;

    if (inst->wave_id >= 0 && inst->wave_id < FB_SFX_MAX_WAVES)
    {
        if (__fb_sfx->waves[inst->wave_id].defined)
            fb_sfxVoiceSetWaveform(voice, __fb_sfx->waves[inst->wave_id].type);
    }

    if (inst->env_id >= 0 && inst->env_id < FB_SFX_MAX_ENVELOPES)
    {
        if (__fb_sfx->envelopes[inst->env_id].defined)
            fb_sfxVoiceSetEnvelope(voice, inst->env_id);
    }
}


/* ------------------------------------------------------------------------- */
/* Query instrument                                                          */
/* ------------------------------------------------------------------------- */

int fb_sfxInstrumentDefined(int id)
{
    if (!__fb_sfx)
        return 0;

    if (id < 0 || id >= FB_SFX_MAX_INSTRUMENTS)
        return 0;

    return __fb_sfx->instruments[id].defined;
}


/* ------------------------------------------------------------------------- */
/* Reset instruments                                                         */
/* ------------------------------------------------------------------------- */

void fb_sfxInstrumentReset(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    for (i = 0; i < FB_SFX_MAX_INSTRUMENTS; i++)
        __fb_sfx->instruments[i].defined = 0;

    SFX_DEBUG("sfx_instrument: instrument table reset");
}


/* end of sfx_instrument.c */
