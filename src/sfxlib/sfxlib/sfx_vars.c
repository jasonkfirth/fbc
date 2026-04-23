/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_vars.c

    Purpose:

        Define global runtime variables used by the FreeBASIC sound
        subsystem.

        This file owns the primary runtime context structure and
        establishes the default configuration of the sound engine.

    Responsibilities:

        • define the global runtime context pointer (__fb_sfx)
        • provide default subsystem configuration values
        • ensure global runtime state is centralized in one location

    This file intentionally does NOT contain:

        • mixer logic
        • audio driver implementations
        • command layer functionality
        • audio synthesis code

    Architectural note:

        All global runtime state for the sound subsystem is defined here.
        Other modules access this state through the pointer __fb_sfx.

        This mirrors the design used by gfxlib2, where the graphics
        runtime context is owned by gfx_vars.c.
*/

#include <stdlib.h>
#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Global runtime context                                                    */
/* ------------------------------------------------------------------------- */

/*
    __fb_sfx

    Pointer to the global runtime sound context.

    The structure itself is allocated during subsystem initialization.
    Keeping the pointer separate allows the runtime to detect whether
    the subsystem has been initialized.
*/

FB_SFXCTX *__fb_sfx = NULL;


/* ------------------------------------------------------------------------- */
/* Default runtime configuration                                             */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxDefaultConfig()

    Initialize the runtime context with safe default values.

    These defaults provide a predictable starting point for the
    sound subsystem and may later be overridden by configuration
    logic or driver requirements.
*/

void fb_sfxDefaultConfig(FB_SFXCTX *ctx)
{
    if (!ctx)
        return;

    memset(ctx, 0, sizeof(FB_SFXCTX));

    ctx->initialized = 0;

    /* standard CD-quality audio */
    ctx->samplerate = FB_SFX_DEFAULT_RATE;

    /* stereo output */
    ctx->output_channels = FB_SFX_DEFAULT_CHANNELS;

    /* default driver buffer size */
    ctx->buffer_size = FB_SFX_DEFAULT_BUFFER;
    ctx->buffer_frames = FB_SFX_DEFAULT_BUFFER;
    ctx->current_channel = 0;
    ctx->master_volume = 1.0f;
    ctx->balance = 0.0f;
    ctx->octave = 4;
    ctx->tempo = 120;

    /* mix buffer state */
    ctx->mix_frames = ctx->buffer_size;

    ctx->mix_read  = 0;
    ctx->mix_write = 0;

    ctx->mixbuffer = NULL;

    ctx->music_playing = -1;
    ctx->music_paused = 0;
    ctx->music_loop = 0;
    ctx->music_pos = 0;

    ctx->driver = NULL;
}


/* ------------------------------------------------------------------------- */
/* Runtime context allocation                                                */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxAllocContext()

    Allocate the runtime context used by the sound subsystem.

    This function ensures that memory allocation and initialization
    of the context structure occurs in a single controlled location.
*/

int fb_sfxAllocContext(void)
{
    if (__fb_sfx)
        return 0;

    __fb_sfx = (FB_SFXCTX *)malloc(sizeof(FB_SFXCTX));

    if (!__fb_sfx)
    {
        SFX_DEBUG("sfx_vars: failed to allocate runtime context");
        return -1;
    }

    fb_sfxDefaultConfig(__fb_sfx);

    SFX_DEBUG("sfx_vars: runtime context allocated");

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Runtime context release                                                   */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxFreeContext()

    Release the runtime context.

    This function should only be called during subsystem shutdown.
*/

void fb_sfxFreeContext(void)
{
    if (!__fb_sfx)
        return;

    free(__fb_sfx);
    __fb_sfx = NULL;

    SFX_DEBUG("sfx_vars: runtime context released");
}


/* end of sfx_vars.c */
