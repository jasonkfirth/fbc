/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: fb_sfx.h

    Purpose:

        Define the core runtime structures and public interfaces used by
        the FreeBASIC sound subsystem.

        This header plays the same architectural role as fb_gfx.h in
        gfxlib2.  It defines the global runtime context, driver interface,
        and shared constants used throughout the library.

    Responsibilities:

        • define the global audio runtime state
        • define the platform driver interface
        • define common audio constants and formats
        • provide forward declarations used across the subsystem

    This file intentionally does NOT contain:

        • audio synthesis implementation
        • mixer implementation
        • platform driver implementations
        • command logic

    Those components are implemented in separate modules.
*/

#ifndef __FB_SFX_H__
#define __FB_SFX_H__

#ifdef __cplusplus
extern "C" {
#endif


/* ------------------------------------------------------------------------- */
/* Forward declarations                                                      */
/* ------------------------------------------------------------------------- */

typedef struct SFXDRIVER SFXDRIVER;
typedef SFXDRIVER FB_SFX_DRIVER;


/* ------------------------------------------------------------------------- */
/* Audio format constants                                                    */
/* ------------------------------------------------------------------------- */

/*
    Internal mixer format

    The mixer always produces signed 32-bit samples internally.
    Drivers may convert this format to match the platform API.
*/

#define FB_SFX_INTERNAL_BITS      32
#define FB_SFX_INTERNAL_CHANNELS  2


/* ------------------------------------------------------------------------- */
/* Default runtime configuration                                             */
/* ------------------------------------------------------------------------- */

#define FB_SFX_DEFAULT_RATE      44100
#define FB_SFX_DEFAULT_CHANNELS  2
#define FB_SFX_DEFAULT_BUFFER    8192


/* ------------------------------------------------------------------------- */
/* Voice limits                                                              */
/* ------------------------------------------------------------------------- */

/*
    Maximum simultaneous voices produced by the mixer.

    This includes:
        • music playback
        • sound effects
        • generated tones
*/

#define FB_SFX_MAX_VOICES 64


/* ------------------------------------------------------------------------- */
/* Channel limits                                                            */
/* ------------------------------------------------------------------------- */

#define FB_SFX_MAX_CHANNELS 16
#define FB_SFX_MAX_WAVES 16
#define FB_SFX_MAX_ENVELOPES 16
#define FB_SFX_MAX_INSTRUMENTS 16
#define FB_SFX_MAX_MUSIC 16
#define FB_SFX_MAX_SFX 64


/* ------------------------------------------------------------------------- */
/* Ring buffer sizes                                                         */
/* ------------------------------------------------------------------------- */

#define FB_SFX_CAPTURE_BUFFER  65536
#define FB_SFX_STREAM_BUFFER   65536


/* ------------------------------------------------------------------------- */
/* Waveform and envelope constants                                           */
/* ------------------------------------------------------------------------- */

#define FB_SFX_WAVE_SINE      0
#define FB_SFX_WAVE_SQUARE    1
#define FB_SFX_WAVE_TRIANGLE  2
#define FB_SFX_WAVE_SAW       3
#define FB_SFX_WAVE_NOISE     4

#define FB_SFX_VOICE_TONE   0
#define FB_SFX_VOICE_SOUND  1
#define FB_SFX_VOICE_SFX    2
#define FB_SFX_VOICE_MUSIC  3
#define FB_SFX_VOICE_PLAY   4
#define FB_SFX_VOICE_NOISE  5

#define FB_SFX_ENV_ATTACK   0
#define FB_SFX_ENV_DECAY    1
#define FB_SFX_ENV_SUSTAIN  2
#define FB_SFX_ENV_RELEASE  3


/* ------------------------------------------------------------------------- */
/* Shared table structures                                                   */
/* ------------------------------------------------------------------------- */

typedef struct FB_SFX_WAVE
{
    int type;
    int defined;
} FB_SFX_WAVE;

typedef struct FB_SFXENVELOPE
{
    int defined;
    float attack;
    float decay;
    float sustain;
    float release;
} FB_SFXENVELOPE;

typedef FB_SFXENVELOPE FB_SFX_ENVELOPE;

typedef struct FB_SFX_INSTRUMENT
{
    int wave_id;
    int env_id;
    int defined;
} FB_SFX_INSTRUMENT;

typedef struct FB_SFX_ASSET
{
    unsigned char *data;
    int size;
    int loaded;
    char name[260];
} FB_SFX_ASSET;


/* ------------------------------------------------------------------------- */
/* Audio voice structure                                                     */
/* ------------------------------------------------------------------------- */

/*
    Voice

    Represents a single sound source being mixed.

    A voice may represent:
        • a tone generator
        • a waveform
        • a streamed audio source
        • a sound effect
*/

typedef struct FB_SFXVOICE
{
    int active;

    int channel;

    float volume;
    float pan;

    int frequency;
    float phase;
    int waveform;
    int type;
    int sfx_id;
    int paused;
    int loop;

    int position;
    int pos;
    int length;
    int start_delay;
    int hard_stop;
    int envelope;
    int env_state;
    float env_level;
    int env_time;

    const float *data;

} FB_SFXVOICE;


/* ------------------------------------------------------------------------- */
/* Audio channel structure                                                   */
/* ------------------------------------------------------------------------- */

typedef struct FB_SFXCHANNEL
{
    float volume;
    float pan;
    int mute;

    int instrument;

} FB_SFXCHANNEL;


/* ------------------------------------------------------------------------- */
/* Capture buffer                                                            */
/* ------------------------------------------------------------------------- */

typedef struct FB_SFXCAPTURE
{
    int enabled;

    int rate;
    int channels;

    int write_pos;
    int read_pos;

    short buffer[FB_SFX_CAPTURE_BUFFER];

} FB_SFXCAPTURE;


/* ------------------------------------------------------------------------- */
/* Runtime audio context                                                     */
/* ------------------------------------------------------------------------- */

/*
    FB_SFXCTX

    Global runtime state for the sound subsystem.

    This structure is equivalent to __fb_gfx used by gfxlib2.

    It is allocated during runtime initialization and shared across
    all components of the sound system.
*/

typedef struct FB_SFXCTX
{
    int initialized;

    int samplerate;
    int output_channels;
    int buffer_frames;

    int buffer_size;
    int current_channel;
    float master_volume;
    float balance;
    int octave;
    int tempo;

    /* ------------------------------------------------------------------ */
    /* Mixer output buffer                                                */
    /* ------------------------------------------------------------------ */

    int mix_frames;

    int mix_read;
    int mix_write;

    float *mixbuffer;

    /* ------------------------------------------------------------------ */
    /* Voice system                                                       */
    /* ------------------------------------------------------------------ */

    FB_SFXVOICE voices[FB_SFX_MAX_VOICES];

    /* ------------------------------------------------------------------ */
    /* Channel state                                                      */
    /* ------------------------------------------------------------------ */

    FB_SFXCHANNEL channels[FB_SFX_MAX_CHANNELS];
    FB_SFX_WAVE waves[FB_SFX_MAX_WAVES];
    FB_SFXENVELOPE envelopes[FB_SFX_MAX_ENVELOPES];
    FB_SFX_INSTRUMENT instruments[FB_SFX_MAX_INSTRUMENTS];
    FB_SFX_ASSET music[FB_SFX_MAX_MUSIC];
    FB_SFX_ASSET sfx[FB_SFX_MAX_SFX];
    int music_playing;
    int music_paused;
    int music_loop;
    int music_pos;

    /* ------------------------------------------------------------------ */
    /* Capture system                                                     */
    /* ------------------------------------------------------------------ */

    FB_SFXCAPTURE capture;

    /* ------------------------------------------------------------------ */
    /* Platform driver                                                    */
    /* ------------------------------------------------------------------ */

    const struct SFXDRIVER *driver;

} FB_SFXCTX;


/* ------------------------------------------------------------------------- */
/* Global runtime pointer                                                    */
/* ------------------------------------------------------------------------- */

extern FB_SFXCTX *__fb_sfx;


/* ------------------------------------------------------------------------- */
/* Runtime control                                                           */
/* ------------------------------------------------------------------------- */

int  fb_sfxInit(void);
void fb_sfxExit(void);


/* ------------------------------------------------------------------------- */
/* Mixer interface                                                           */
/* ------------------------------------------------------------------------- */

void fb_sfxMixFrame(float *buffer, int frames);


/* ------------------------------------------------------------------------- */
/* Driver list                                                               */
/* ------------------------------------------------------------------------- */

extern const SFXDRIVER *__fb_sfx_drivers_list[];


#ifdef __cplusplus
}
#endif

#endif


/* end of fb_sfx.h */
