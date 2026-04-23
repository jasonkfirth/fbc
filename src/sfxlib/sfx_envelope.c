/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_envelope.c

    Purpose:

        Implement the ADSR envelope system used by the sound engine.

        Envelopes control how the amplitude of a voice evolves
        over time.

    Responsibilities:

        • envelope initialization
        • envelope definition storage
        • envelope state updates
        • envelope amplitude calculation

    This file intentionally does NOT contain:

        • mixer logic
        • oscillator generation
        • driver interaction
        • command parsing

    Envelope model:

        Attack  → ramp from 0 to 1
        Decay   → ramp from 1 to sustain level
        Sustain → constant level
        Release → ramp to 0 after note-off
*/

#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Envelope initialization                                                   */
/* ------------------------------------------------------------------------- */

void fb_sfxEnvelopeInit(void)
{
    int i;

    if (!__fb_sfx)
        return;

    for (i = 0; i < FB_SFX_MAX_ENVELOPES; i++)
    {
        FB_SFXENVELOPE *env = &__fb_sfx->envelopes[i];

        memset(env, 0, sizeof(FB_SFXENVELOPE));

        env->attack  = 0.01f;
        env->decay   = 0.10f;
        env->sustain = 0.8f;
        env->release = 0.20f;
    }

    SFX_DEBUG("sfx_envelope: initialized %d envelopes",
              FB_SFX_MAX_ENVELOPES);
}


/* ------------------------------------------------------------------------- */
/* Envelope definition                                                       */
/* ------------------------------------------------------------------------- */

void fb_sfxEnvelopeDefine(
    int id,
    float attack,
    float decay,
    float sustain,
    float release
)
{
    FB_SFXENVELOPE *env;

    if (!__fb_sfx)
        return;

    if (id < 0 || id >= FB_SFX_MAX_ENVELOPES)
        return;

    if (attack  < 0.0f) attack  = 0.0f;
    if (decay   < 0.0f) decay   = 0.0f;
    if (release < 0.0f) release = 0.0f;

    if (sustain < 0.0f) sustain = 0.0f;
    if (sustain > 1.0f) sustain = 1.0f;

    env = &__fb_sfx->envelopes[id];

    env->attack  = attack;
    env->decay   = decay;
    env->sustain = sustain;
    env->release = release;

    SFX_DEBUG("sfx_envelope: define id=%d A=%f D=%f S=%f R=%f",
              id, attack, decay, sustain, release);
}


/* ------------------------------------------------------------------------- */
/* Envelope assignment                                                       */
/* ------------------------------------------------------------------------- */

void fb_sfxVoiceSetEnvelope(FB_SFXVOICE *voice, int id)
{
    if (!voice)
        return;

    if (!__fb_sfx)
        return;

    if (id < 0 || id >= FB_SFX_MAX_ENVELOPES)
        return;

    voice->envelope = id;
    voice->env_state = FB_SFX_ENV_ATTACK;
    voice->env_level = 0.0f;
    voice->env_time = 0;

    SFX_DEBUG("sfx_envelope: voice assigned envelope %d", id);
}


/* ------------------------------------------------------------------------- */
/* Envelope note-off                                                         */
/* ------------------------------------------------------------------------- */

void fb_sfxEnvelopeRelease(FB_SFXVOICE *voice)
{
    if (!voice)
        return;

    voice->env_state = FB_SFX_ENV_RELEASE;
    voice->env_time = 0;
}


/* ------------------------------------------------------------------------- */
/* Envelope processing                                                       */
/* ------------------------------------------------------------------------- */

float fb_sfxEnvelopeProcess(FB_SFXVOICE *voice, float dt)
{
    FB_SFXENVELOPE *env;
    float level;

    if (!voice)
        return 0.0f;

    if (!__fb_sfx)
        return 0.0f;

    env = &__fb_sfx->envelopes[voice->envelope];

    level = voice->env_level;

    switch (voice->env_state)
    {
        /* ------------------------------------------------------------- */
        /* Attack                                                        */
        /* ------------------------------------------------------------- */

        case FB_SFX_ENV_ATTACK:

            if (env->attack <= 0.0f)
            {
                level = 1.0f;
                voice->env_state = FB_SFX_ENV_DECAY;
            }
            else
            {
                level += dt / env->attack;

                if (level >= 1.0f)
                {
                    level = 1.0f;
                    voice->env_state = FB_SFX_ENV_DECAY;
                }
            }

            break;


        /* ------------------------------------------------------------- */
        /* Decay                                                         */
        /* ------------------------------------------------------------- */

        case FB_SFX_ENV_DECAY:

            if (env->decay <= 0.0f)
            {
                level = env->sustain;
                voice->env_state = FB_SFX_ENV_SUSTAIN;
            }
            else
            {
                level -= dt * (1.0f - env->sustain) / env->decay;

                if (level <= env->sustain)
                {
                    level = env->sustain;
                    voice->env_state = FB_SFX_ENV_SUSTAIN;
                }
            }

            break;


        /* ------------------------------------------------------------- */
        /* Sustain                                                       */
        /* ------------------------------------------------------------- */

        case FB_SFX_ENV_SUSTAIN:

            level = env->sustain;
            break;


        /* ------------------------------------------------------------- */
        /* Release                                                       */
        /* ------------------------------------------------------------- */

        case FB_SFX_ENV_RELEASE:

            if (env->release <= 0.0f)
            {
                level = 0.0f;
                voice->active = 0;
            }
            else
            {
                level -= dt / env->release;

                if (level <= 0.0f)
                {
                    level = 0.0f;
                    voice->active = 0;
                }
            }

            break;
    }

    voice->env_level = level;

    return level;
}


/* end of sfx_envelope.c */
