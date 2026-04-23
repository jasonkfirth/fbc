/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_envelope_cmd.c

    Purpose:

        Implement the BASIC ENVELOPE command.

        The command defines an ADSR envelope that can later
        be assigned to voices or instruments.

    Responsibilities:

        • define ADSR envelope parameters
        • validate envelope ranges
        • store envelope definitions

    This file intentionally does NOT contain:

        • envelope processing
        • mixer logic
        • oscillator generation
        • driver interaction

    Architectural overview:

        ENVELOPE command
             │
             ▼
        envelope definition table
             │
             ▼
        voice synthesis system
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Envelope limits                                                           */
/* ------------------------------------------------------------------------- */

#define FB_SFX_ENV_ATTACK_MAX   10.0f
#define FB_SFX_ENV_DECAY_MAX    10.0f
#define FB_SFX_ENV_RELEASE_MAX  10.0f

#define FB_SFX_ENV_SUSTAIN_MIN  0.0f
#define FB_SFX_ENV_SUSTAIN_MAX  1.0f


/* ------------------------------------------------------------------------- */
/* ENVELOPE command                                                          */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxEnvelopeCmd()

    Define an ADSR envelope.

    Parameters:

        id        envelope identifier
        attack    attack time (seconds)
        decay     decay time (seconds)
        sustain   sustain level (0.0 – 1.0)
        release   release time (seconds)
*/

void fb_sfxEnvelopeCmd(
    int id,
    float attack,
    float decay,
    float sustain,
    float release
)
{
    FB_SFX_ENVELOPE *env;

    if (!fb_sfxEnsureInitialized())
        return;

    if (id < 0 || id >= FB_SFX_MAX_ENVELOPES)
        return;

    /* clamp parameters */

    if (attack < 0.0f)
        attack = 0.0f;
    if (attack > FB_SFX_ENV_ATTACK_MAX)
        attack = FB_SFX_ENV_ATTACK_MAX;

    if (decay < 0.0f)
        decay = 0.0f;
    if (decay > FB_SFX_ENV_DECAY_MAX)
        decay = FB_SFX_ENV_DECAY_MAX;

    if (release < 0.0f)
        release = 0.0f;
    if (release > FB_SFX_ENV_RELEASE_MAX)
        release = FB_SFX_ENV_RELEASE_MAX;

    if (sustain < FB_SFX_ENV_SUSTAIN_MIN)
        sustain = FB_SFX_ENV_SUSTAIN_MIN;
    if (sustain > FB_SFX_ENV_SUSTAIN_MAX)
        sustain = FB_SFX_ENV_SUSTAIN_MAX;

    env = &__fb_sfx->envelopes[id];

    env->attack  = attack;
    env->decay   = decay;
    env->sustain = sustain;
    env->release = release;

    env->defined = 1;

    SFX_DEBUG(
        "sfx_envelope_cmd: id=%d A=%f D=%f S=%f R=%f",
        id, attack, decay, sustain, release
    );
}


/* ------------------------------------------------------------------------- */
/* Query envelope definition                                                 */
/* ------------------------------------------------------------------------- */

int fb_sfxEnvelopeDefined(int id)
{
    if (!__fb_sfx)
        return 0;

    if (id < 0 || id >= FB_SFX_MAX_ENVELOPES)
        return 0;

    return __fb_sfx->envelopes[id].defined;
}


/* ------------------------------------------------------------------------- */
/* Reset envelopes                                                           */
/* ------------------------------------------------------------------------- */

void fb_sfxEnvelopeCmdReset(void)
{
    int i;

    if (!fb_sfxEnsureInitialized())
        return;

    for (i = 0; i < FB_SFX_MAX_ENVELOPES; i++)
        __fb_sfx->envelopes[i].defined = 0;

    SFX_DEBUG("sfx_envelope_cmd: all envelopes reset");
}


/* end of sfx_envelope_cmd.c */
