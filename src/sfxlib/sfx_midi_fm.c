/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_midi_fm.c

    Purpose:

        Provide a small software MIDI synthesizer for systems where the
        platform MIDI output cannot be opened.

    Responsibilities:

        - maintain General MIDI channel and controller state
        - allocate a bounded set of polyphonic software voices
        - synthesize inexpensive two-operator FM instrument presets
        - provide a small noise-assisted percussion set on channel 10
        - add the generated stereo waveform to the normal sfxlib mix

    This file intentionally does NOT contain:

        - Standard MIDI File parsing or event timing
        - sample-based General MIDI instruments
        - platform MIDI device access
        - dynamic memory allocation

    Audio path:

        MIDI events -> FM voices -> mixer -> ring buffer -> audio driver

    Design note:

        This is a last-resort musical backend, not a General MIDI emulator.
        Each General MIDI program number has a compact preset with its own
        envelope and modulation shape. The fixed voice and sine-table sizes
        keep its memory and CPU use predictable on small targets.
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <math.h>
#include <stdint.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* Synthesizer limits and constants                                          */
/* ------------------------------------------------------------------------- */

#define FB_SFX_MIDI_FM_CHANNELS       16
#define FB_SFX_MIDI_FM_VOICES         32
#define FB_SFX_MIDI_FM_SINE_SAMPLES 1024
#define FB_SFX_MIDI_FM_DRUM_CHANNEL    9

#define FB_SFX_MIDI_FM_ATTACK          1
#define FB_SFX_MIDI_FM_DECAY           2
#define FB_SFX_MIDI_FM_SUSTAIN         3
#define FB_SFX_MIDI_FM_RELEASE         4

#define FB_SFX_MIDI_FM_TWO_PI 6.28318530717958647692f


/* ------------------------------------------------------------------------- */
/* Synthesizer state                                                         */
/* ------------------------------------------------------------------------- */

typedef struct FB_SFX_MIDI_FM_TIMBRE
{
    float modulator_ratio;
    float modulation_index;
    float attack_seconds;
    float decay_seconds;
    float sustain_level;
    float release_seconds;
    float output_gain;
    float velocity_brightness;
} FB_SFX_MIDI_FM_TIMBRE;

typedef struct FB_SFX_MIDI_FM_CHANNEL
{
    unsigned char program;
    unsigned char volume;
    unsigned char expression;
    unsigned char pan;
    unsigned char sustain;
    unsigned short pitch_bend;
} FB_SFX_MIDI_FM_CHANNEL;

typedef struct FB_SFX_MIDI_FM_VOICE
{
    int active;
    int key_down;
    int sustained;
    int percussion;
    int envelope_state;
    int channel;
    int note;
    int program;
    int drum_frames;

    unsigned long age;
    uint32_t noise_state;

    float frequency;
    float carrier_phase;
    float modulator_phase;
    float modulator_ratio;
    float modulation_index;
    float envelope_level;
    float release_step;
    float velocity_gain;
    float output_gain;
    float drum_decay;
    float noise_mix;
} FB_SFX_MIDI_FM_VOICE;

typedef struct FB_SFX_MIDI_FM_STATE
{
    int active;
    int paused;
    int sine_table_ready;

    unsigned long next_age;

    float sine_table[FB_SFX_MIDI_FM_SINE_SAMPLES];
    FB_SFX_MIDI_FM_CHANNEL channels[FB_SFX_MIDI_FM_CHANNELS];
    FB_SFX_MIDI_FM_VOICE voices[FB_SFX_MIDI_FM_VOICES];
} FB_SFX_MIDI_FM_STATE;

static FB_SFX_MIDI_FM_STATE g_fb_sfx_midi_fm;


/*
    General MIDI program mapping

    Every program has its own compact two-operator preset. This is not an
    attempt to reproduce sampled instruments. The goal is to make program
    changes musically useful and broadly recognizable without adding files,
    allocations, or a large synthesizer dependency.

    velocity_brightness controls how strongly note velocity affects the FM
    modulation index. The renderer also reduces that index near Nyquist so
    these presets remain controlled at low output sample rates.
*/

#define FB_SFX_MIDI_FM_PRESET(ratio, index, attack, decay, sustain, release, gain, velocity) \
    { ratio, index, attack, decay, sustain, release, gain, velocity }

static const FB_SFX_MIDI_FM_TIMBRE g_fb_sfx_midi_fm_timbres[128] =
{
    /* Piano */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.55f, 0.004f, 0.55f, 0.25f, 0.28f, 0.95f, 0.55f), /*   0 Acoustic Grand Piano */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.90f, 0.003f, 0.42f, 0.22f, 0.24f, 0.94f, 0.65f), /*   1 Bright Acoustic Piano */
    FB_SFX_MIDI_FM_PRESET(1.50f, 1.25f, 0.006f, 0.65f, 0.32f, 0.35f, 0.92f, 0.45f), /*   2 Electric Grand Piano */
    FB_SFX_MIDI_FM_PRESET(3.00f, 2.20f, 0.002f, 0.34f, 0.18f, 0.18f, 0.86f, 0.70f), /*   3 Honky-tonk Piano */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.85f, 0.008f, 0.55f, 0.42f, 0.45f, 0.90f, 0.35f), /*   4 Electric Piano 1 */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.10f, 0.006f, 0.50f, 0.38f, 0.40f, 0.90f, 0.50f), /*   5 Electric Piano 2 */
    FB_SFX_MIDI_FM_PRESET(4.00f, 1.75f, 0.002f, 0.30f, 0.12f, 0.10f, 0.82f, 0.50f), /*   6 Harpsichord */
    FB_SFX_MIDI_FM_PRESET(3.00f, 2.05f, 0.002f, 0.24f, 0.18f, 0.10f, 0.85f, 0.65f), /*   7 Clavinet */

    /* Chromatic percussion */
    FB_SFX_MIDI_FM_PRESET(3.00f, 1.45f, 0.003f, 0.70f, 0.15f, 0.30f, 0.85f, 0.50f), /*   8 Celesta */
    FB_SFX_MIDI_FM_PRESET(3.50f, 3.50f, 0.001f, 1.10f, 0.05f, 0.50f, 0.72f, 0.60f), /*   9 Glockenspiel */
    FB_SFX_MIDI_FM_PRESET(3.00f, 2.40f, 0.002f, 0.90f, 0.10f, 0.45f, 0.75f, 0.50f), /*  10 Music Box */
    FB_SFX_MIDI_FM_PRESET(4.00f, 1.40f, 0.006f, 0.80f, 0.38f, 1.00f, 0.82f, 0.35f), /*  11 Vibraphone */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.25f, 0.002f, 0.35f, 0.12f, 0.15f, 0.88f, 0.45f), /*  12 Marimba */
    FB_SFX_MIDI_FM_PRESET(3.00f, 1.90f, 0.001f, 0.18f, 0.03f, 0.06f, 0.84f, 0.55f), /*  13 Xylophone */
    FB_SFX_MIDI_FM_PRESET(2.76f, 4.20f, 0.002f, 1.40f, 0.15f, 1.20f, 0.68f, 0.55f), /*  14 Tubular Bells */
    FB_SFX_MIDI_FM_PRESET(2.00f, 2.00f, 0.002f, 0.45f, 0.18f, 0.20f, 0.82f, 0.50f), /*  15 Dulcimer */

    /* Organ */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.32f, 0.012f, 0.08f, 0.92f, 0.16f, 0.86f, 0.20f), /*  16 Drawbar Organ */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.75f, 0.004f, 0.16f, 0.78f, 0.12f, 0.84f, 0.35f), /*  17 Percussive Organ */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.90f, 0.008f, 0.10f, 0.88f, 0.14f, 0.86f, 0.40f), /*  18 Rock Organ */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.38f, 0.045f, 0.18f, 0.94f, 0.55f, 0.82f, 0.18f), /*  19 Church Organ */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.58f, 0.018f, 0.12f, 0.90f, 0.22f, 0.84f, 0.25f), /*  20 Reed Organ */
    FB_SFX_MIDI_FM_PRESET(1.00f, 1.10f, 0.020f, 0.14f, 0.78f, 0.20f, 0.83f, 0.35f), /*  21 Accordion */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.42f, 0.014f, 0.12f, 0.72f, 0.16f, 0.80f, 0.45f), /*  22 Harmonica */
    FB_SFX_MIDI_FM_PRESET(1.50f, 1.28f, 0.016f, 0.13f, 0.80f, 0.18f, 0.82f, 0.38f), /*  23 Tango Accordion */

    /* Guitar */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.85f, 0.005f, 0.55f, 0.20f, 0.25f, 0.90f, 0.50f), /*  24 Acoustic Guitar (nylon) */
    FB_SFX_MIDI_FM_PRESET(3.00f, 1.25f, 0.004f, 0.48f, 0.18f, 0.22f, 0.86f, 0.55f), /*  25 Acoustic Guitar (steel) */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.65f, 0.006f, 0.38f, 0.48f, 0.28f, 0.86f, 0.35f), /*  26 Electric Guitar (jazz) */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.92f, 0.004f, 0.32f, 0.44f, 0.22f, 0.86f, 0.45f), /*  27 Electric Guitar (clean) */
    FB_SFX_MIDI_FM_PRESET(4.00f, 2.20f, 0.002f, 0.12f, 0.05f, 0.06f, 0.72f, 0.70f), /*  28 Electric Guitar (muted) */
    FB_SFX_MIDI_FM_PRESET(1.00f, 2.35f, 0.003f, 0.22f, 0.55f, 0.16f, 0.72f, 0.62f), /*  29 Overdriven Guitar */
    FB_SFX_MIDI_FM_PRESET(1.00f, 3.55f, 0.002f, 0.18f, 0.58f, 0.14f, 0.64f, 0.70f), /*  30 Distortion Guitar */
    FB_SFX_MIDI_FM_PRESET(4.00f, 2.05f, 0.002f, 0.70f, 0.15f, 0.50f, 0.72f, 0.45f), /*  31 Guitar Harmonics */

    /* Bass */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.82f, 0.004f, 0.32f, 0.36f, 0.14f, 0.94f, 0.42f), /*  32 Acoustic Bass */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.68f, 0.004f, 0.26f, 0.48f, 0.12f, 0.94f, 0.35f), /*  33 Electric Bass (finger) */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.12f, 0.002f, 0.22f, 0.42f, 0.10f, 0.90f, 0.55f), /*  34 Electric Bass (pick) */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.46f, 0.012f, 0.30f, 0.58f, 0.22f, 0.92f, 0.25f), /*  35 Fretless Bass */
    FB_SFX_MIDI_FM_PRESET(2.00f, 2.45f, 0.001f, 0.16f, 0.24f, 0.08f, 0.78f, 0.70f), /*  36 Slap Bass 1 */
    FB_SFX_MIDI_FM_PRESET(3.00f, 2.75f, 0.001f, 0.13f, 0.20f, 0.07f, 0.74f, 0.75f), /*  37 Slap Bass 2 */
    FB_SFX_MIDI_FM_PRESET(1.00f, 3.10f, 0.003f, 0.20f, 0.52f, 0.14f, 0.72f, 0.65f), /*  38 Synth Bass 1 */
    FB_SFX_MIDI_FM_PRESET(0.50f, 3.75f, 0.004f, 0.24f, 0.46f, 0.16f, 0.68f, 0.72f), /*  39 Synth Bass 2 */

    /* Strings */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.58f, 0.055f, 0.24f, 0.76f, 0.38f, 0.84f, 0.30f), /*  40 Violin */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.48f, 0.065f, 0.26f, 0.78f, 0.42f, 0.86f, 0.25f), /*  41 Viola */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.42f, 0.072f, 0.28f, 0.80f, 0.46f, 0.90f, 0.22f), /*  42 Cello */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.34f, 0.082f, 0.30f, 0.82f, 0.50f, 0.92f, 0.20f), /*  43 Contrabass */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.72f, 0.038f, 0.20f, 0.72f, 0.34f, 0.80f, 0.35f), /*  44 Tremolo Strings */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.45f, 0.002f, 0.24f, 0.08f, 0.10f, 0.86f, 0.52f), /*  45 Pizzicato Strings */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.88f, 0.004f, 0.68f, 0.28f, 0.65f, 0.82f, 0.38f), /*  46 Orchestral Harp */
    FB_SFX_MIDI_FM_PRESET(1.00f, 2.55f, 0.001f, 0.36f, 0.10f, 0.12f, 0.74f, 0.68f), /*  47 Timpani */

    /* Ensemble */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.52f, 0.095f, 0.30f, 0.74f, 0.48f, 0.82f, 0.28f), /*  48 String Ensemble 1 */
    FB_SFX_MIDI_FM_PRESET(2.01f, 0.68f, 0.125f, 0.36f, 0.70f, 0.58f, 0.78f, 0.32f), /*  49 String Ensemble 2 */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.92f, 0.075f, 0.24f, 0.72f, 0.38f, 0.78f, 0.42f), /*  50 Synth Strings 1 */
    FB_SFX_MIDI_FM_PRESET(0.50f, 1.22f, 0.105f, 0.30f, 0.68f, 0.46f, 0.74f, 0.48f), /*  51 Synth Strings 2 */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.42f, 0.115f, 0.26f, 0.82f, 0.52f, 0.86f, 0.20f), /*  52 Choir Aahs */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.36f, 0.135f, 0.30f, 0.84f, 0.55f, 0.84f, 0.18f), /*  53 Voice Oohs */
    FB_SFX_MIDI_FM_PRESET(1.50f, 0.78f, 0.090f, 0.22f, 0.76f, 0.42f, 0.80f, 0.32f), /*  54 Synth Voice */
    FB_SFX_MIDI_FM_PRESET(2.50f, 2.35f, 0.002f, 0.28f, 0.22f, 0.16f, 0.72f, 0.62f), /*  55 Orchestra Hit */

    /* Brass */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.32f, 0.012f, 0.16f, 0.74f, 0.18f, 0.80f, 0.52f), /*  56 Trumpet */
    FB_SFX_MIDI_FM_PRESET(1.00f, 1.02f, 0.018f, 0.18f, 0.78f, 0.22f, 0.84f, 0.44f), /*  57 Trombone */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.72f, 0.026f, 0.20f, 0.82f, 0.28f, 0.90f, 0.35f), /*  58 Tuba */
    FB_SFX_MIDI_FM_PRESET(3.00f, 2.05f, 0.008f, 0.14f, 0.58f, 0.14f, 0.68f, 0.65f), /*  59 Muted Trumpet */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.62f, 0.045f, 0.24f, 0.84f, 0.38f, 0.86f, 0.25f), /*  60 French Horn */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.48f, 0.016f, 0.16f, 0.76f, 0.20f, 0.78f, 0.50f), /*  61 Brass Section */
    FB_SFX_MIDI_FM_PRESET(2.00f, 2.55f, 0.006f, 0.13f, 0.64f, 0.16f, 0.68f, 0.65f), /*  62 Synth Brass 1 */
    FB_SFX_MIDI_FM_PRESET(0.50f, 3.05f, 0.010f, 0.18f, 0.60f, 0.18f, 0.64f, 0.72f), /*  63 Synth Brass 2 */

    /* Reed */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.08f, 0.018f, 0.16f, 0.72f, 0.20f, 0.82f, 0.42f), /*  64 Soprano Sax */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.98f, 0.020f, 0.18f, 0.74f, 0.22f, 0.84f, 0.38f), /*  65 Alto Sax */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.90f, 0.022f, 0.20f, 0.76f, 0.24f, 0.86f, 0.36f), /*  66 Tenor Sax */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.78f, 0.026f, 0.22f, 0.78f, 0.28f, 0.88f, 0.32f), /*  67 Baritone Sax */
    FB_SFX_MIDI_FM_PRESET(3.00f, 1.42f, 0.016f, 0.14f, 0.68f, 0.18f, 0.78f, 0.48f), /*  68 Oboe */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.18f, 0.024f, 0.18f, 0.74f, 0.24f, 0.82f, 0.40f), /*  69 English Horn */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.86f, 0.028f, 0.20f, 0.78f, 0.28f, 0.88f, 0.32f), /*  70 Bassoon */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.92f, 0.014f, 0.15f, 0.76f, 0.18f, 0.84f, 0.35f), /*  71 Clarinet */

    /* Pipe */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.82f, 0.020f, 0.12f, 0.82f, 0.20f, 0.78f, 0.32f), /*  72 Piccolo */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.36f, 0.038f, 0.16f, 0.88f, 0.28f, 0.88f, 0.18f), /*  73 Flute */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.58f, 0.026f, 0.14f, 0.84f, 0.22f, 0.84f, 0.24f), /*  74 Recorder */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.68f, 0.045f, 0.18f, 0.80f, 0.30f, 0.82f, 0.28f), /*  75 Pan Flute */
    FB_SFX_MIDI_FM_PRESET(4.00f, 1.62f, 0.055f, 0.20f, 0.62f, 0.32f, 0.70f, 0.45f), /*  76 Blown Bottle */
    FB_SFX_MIDI_FM_PRESET(2.50f, 1.12f, 0.060f, 0.24f, 0.72f, 0.36f, 0.76f, 0.35f), /*  77 Shakuhachi */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.52f, 0.012f, 0.10f, 0.90f, 0.16f, 0.82f, 0.20f), /*  78 Whistle */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.46f, 0.024f, 0.14f, 0.86f, 0.22f, 0.86f, 0.20f), /*  79 Ocarina */

    /* Synth lead */
    FB_SFX_MIDI_FM_PRESET(1.00f, 2.45f, 0.003f, 0.10f, 0.72f, 0.14f, 0.70f, 0.62f), /*  80 Lead 1 (square) */
    FB_SFX_MIDI_FM_PRESET(2.00f, 2.85f, 0.003f, 0.11f, 0.68f, 0.14f, 0.68f, 0.68f), /*  81 Lead 2 (sawtooth) */
    FB_SFX_MIDI_FM_PRESET(3.00f, 1.35f, 0.008f, 0.14f, 0.76f, 0.18f, 0.74f, 0.42f), /*  82 Lead 3 (calliope) */
    FB_SFX_MIDI_FM_PRESET(4.00f, 2.05f, 0.002f, 0.12f, 0.66f, 0.12f, 0.68f, 0.62f), /*  83 Lead 4 (chiff) */
    FB_SFX_MIDI_FM_PRESET(1.00f, 3.85f, 0.004f, 0.16f, 0.62f, 0.16f, 0.60f, 0.72f), /*  84 Lead 5 (charang) */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.66f, 0.030f, 0.18f, 0.82f, 0.28f, 0.82f, 0.25f), /*  85 Lead 6 (voice) */
    FB_SFX_MIDI_FM_PRESET(1.50f, 2.05f, 0.006f, 0.14f, 0.70f, 0.18f, 0.68f, 0.55f), /*  86 Lead 7 (fifths) */
    FB_SFX_MIDI_FM_PRESET(0.50f, 2.85f, 0.004f, 0.18f, 0.64f, 0.16f, 0.66f, 0.64f), /*  87 Lead 8 (bass + lead) */

    /* Synth pad */
    FB_SFX_MIDI_FM_PRESET(0.50f, 1.12f, 0.220f, 0.42f, 0.76f, 0.62f, 0.74f, 0.32f), /*  88 Pad 1 (new age) */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.48f, 0.180f, 0.36f, 0.82f, 0.58f, 0.82f, 0.18f), /*  89 Pad 2 (warm) */
    FB_SFX_MIDI_FM_PRESET(2.00f, 1.22f, 0.095f, 0.28f, 0.72f, 0.42f, 0.76f, 0.40f), /*  90 Pad 3 (polysynth) */
    FB_SFX_MIDI_FM_PRESET(3.00f, 0.58f, 0.150f, 0.32f, 0.80f, 0.55f, 0.80f, 0.22f), /*  91 Pad 4 (choir) */
    FB_SFX_MIDI_FM_PRESET(1.50f, 0.82f, 0.240f, 0.46f, 0.74f, 0.70f, 0.78f, 0.28f), /*  92 Pad 5 (bowed) */
    FB_SFX_MIDI_FM_PRESET(3.70f, 2.85f, 0.120f, 0.38f, 0.62f, 0.58f, 0.60f, 0.55f), /*  93 Pad 6 (metallic) */
    FB_SFX_MIDI_FM_PRESET(0.50f, 0.72f, 0.260f, 0.50f, 0.78f, 0.82f, 0.78f, 0.24f), /*  94 Pad 7 (halo) */
    FB_SFX_MIDI_FM_PRESET(0.25f, 2.05f, 0.320f, 0.55f, 0.68f, 0.85f, 0.66f, 0.48f), /*  95 Pad 8 (sweep) */

    /* Synth effects */
    FB_SFX_MIDI_FM_PRESET(3.77f, 3.55f, 0.055f, 0.34f, 0.42f, 0.45f, 0.56f, 0.62f), /*  96 FX 1 (rain) */
    FB_SFX_MIDI_FM_PRESET(0.50f, 1.65f, 0.180f, 0.48f, 0.64f, 0.72f, 0.68f, 0.36f), /*  97 FX 2 (soundtrack) */
    FB_SFX_MIDI_FM_PRESET(4.10f, 4.55f, 0.008f, 0.95f, 0.18f, 1.10f, 0.52f, 0.68f), /*  98 FX 3 (crystal) */
    FB_SFX_MIDI_FM_PRESET(0.75f, 2.25f, 0.160f, 0.42f, 0.58f, 0.64f, 0.62f, 0.46f), /*  99 FX 4 (atmosphere) */
    FB_SFX_MIDI_FM_PRESET(5.00f, 3.05f, 0.018f, 0.38f, 0.34f, 0.45f, 0.56f, 0.58f), /* 100 FX 5 (brightness) */
    FB_SFX_MIDI_FM_PRESET(0.33f, 3.85f, 0.090f, 0.32f, 0.48f, 0.52f, 0.54f, 0.64f), /* 101 FX 6 (goblins) */
    FB_SFX_MIDI_FM_PRESET(2.01f, 2.45f, 0.030f, 0.58f, 0.36f, 0.78f, 0.58f, 0.52f), /* 102 FX 7 (echoes) */
    FB_SFX_MIDI_FM_PRESET(6.50f, 4.85f, 0.025f, 0.46f, 0.30f, 0.58f, 0.48f, 0.72f), /* 103 FX 8 (sci-fi) */

    /* Ethnic */
    FB_SFX_MIDI_FM_PRESET(3.00f, 2.65f, 0.003f, 0.46f, 0.18f, 0.26f, 0.72f, 0.62f), /* 104 Sitar */
    FB_SFX_MIDI_FM_PRESET(2.00f, 2.25f, 0.002f, 0.30f, 0.12f, 0.14f, 0.78f, 0.58f), /* 105 Banjo */
    FB_SFX_MIDI_FM_PRESET(3.00f, 2.05f, 0.002f, 0.26f, 0.10f, 0.12f, 0.78f, 0.55f), /* 106 Shamisen */
    FB_SFX_MIDI_FM_PRESET(2.50f, 1.82f, 0.003f, 0.42f, 0.16f, 0.22f, 0.80f, 0.50f), /* 107 Koto */
    FB_SFX_MIDI_FM_PRESET(4.00f, 1.85f, 0.002f, 0.32f, 0.08f, 0.16f, 0.78f, 0.52f), /* 108 Kalimba */
    FB_SFX_MIDI_FM_PRESET(1.00f, 0.92f, 0.040f, 0.18f, 0.84f, 0.34f, 0.84f, 0.32f), /* 109 Bag Pipe */
    FB_SFX_MIDI_FM_PRESET(2.00f, 0.78f, 0.032f, 0.20f, 0.76f, 0.30f, 0.84f, 0.34f), /* 110 Fiddle */
    FB_SFX_MIDI_FM_PRESET(3.00f, 1.65f, 0.016f, 0.16f, 0.70f, 0.20f, 0.76f, 0.52f), /* 111 Shanai */

    /* Percussive */
    FB_SFX_MIDI_FM_PRESET(4.70f, 4.05f, 0.001f, 0.75f, 0.08f, 0.52f, 0.56f, 0.62f), /* 112 Tinkle Bell */
    FB_SFX_MIDI_FM_PRESET(3.20f, 3.05f, 0.001f, 0.28f, 0.06f, 0.12f, 0.66f, 0.68f), /* 113 Agogo */
    FB_SFX_MIDI_FM_PRESET(2.50f, 1.85f, 0.002f, 0.38f, 0.24f, 0.22f, 0.76f, 0.55f), /* 114 Steel Drums */
    FB_SFX_MIDI_FM_PRESET(4.00f, 2.55f, 0.001f, 0.10f, 0.02f, 0.04f, 0.68f, 0.72f), /* 115 Woodblock */
    FB_SFX_MIDI_FM_PRESET(1.00f, 3.55f, 0.001f, 0.24f, 0.05f, 0.10f, 0.66f, 0.75f), /* 116 Taiko Drum */
    FB_SFX_MIDI_FM_PRESET(1.00f, 2.25f, 0.001f, 0.30f, 0.12f, 0.14f, 0.72f, 0.68f), /* 117 Melodic Tom */
    FB_SFX_MIDI_FM_PRESET(0.50f, 4.25f, 0.001f, 0.20f, 0.08f, 0.08f, 0.58f, 0.78f), /* 118 Synth Drum */
    FB_SFX_MIDI_FM_PRESET(3.33f, 4.85f, 0.260f, 0.42f, 0.40f, 0.16f, 0.48f, 0.65f), /* 119 Reverse Cymbal */

    /* Sound effects */
    FB_SFX_MIDI_FM_PRESET(4.00f, 3.05f, 0.001f, 0.08f, 0.02f, 0.04f, 0.52f, 0.70f), /* 120 Guitar Fret Noise */
    FB_SFX_MIDI_FM_PRESET(5.00f, 1.55f, 0.010f, 0.16f, 0.06f, 0.08f, 0.52f, 0.45f), /* 121 Breath Noise */
    FB_SFX_MIDI_FM_PRESET(0.37f, 4.55f, 0.120f, 0.46f, 0.38f, 0.52f, 0.48f, 0.62f), /* 122 Seashore */
    FB_SFX_MIDI_FM_PRESET(6.00f, 3.55f, 0.004f, 0.20f, 0.18f, 0.16f, 0.48f, 0.72f), /* 123 Bird Tweet */
    FB_SFX_MIDI_FM_PRESET(2.00f, 2.25f, 0.004f, 0.10f, 0.88f, 0.16f, 0.58f, 0.42f), /* 124 Telephone Ring */
    FB_SFX_MIDI_FM_PRESET(0.20f, 4.55f, 0.018f, 0.22f, 0.74f, 0.20f, 0.46f, 0.65f), /* 125 Helicopter */
    FB_SFX_MIDI_FM_PRESET(3.17f, 4.85f, 0.006f, 0.34f, 0.28f, 0.28f, 0.44f, 0.74f), /* 126 Applause */
    FB_SFX_MIDI_FM_PRESET(1.00f, 6.00f, 0.001f, 0.08f, 0.01f, 0.04f, 0.42f, 0.82f)  /* 127 Gunshot */
};

#undef FB_SFX_MIDI_FM_PRESET

static const FB_SFX_MIDI_FM_TIMBRE g_fb_sfx_midi_fm_drum_timbre =
{
    1.25f, 3.60f, 0.001f, 0.18f, 0.08f, 0.10f, 0.72f, 0.70f
};


/* ------------------------------------------------------------------------- */
/* Table and state initialization                                            */
/* ------------------------------------------------------------------------- */

static int fb_sfxMidiFmValidTimbre(const FB_SFX_MIDI_FM_TIMBRE *timbre)
{
    if (!timbre)
        return 0;

    /*
        Written this way, rather than only testing the limits separately,
        so a NaN in a preset also fails validation.
    */

    if (!(timbre->modulator_ratio >= 0.125f &&
          timbre->modulator_ratio <= 8.0f))
    {
        return 0;
    }

    if (!(timbre->modulation_index >= 0.0f &&
          timbre->modulation_index <= 8.0f))
    {
        return 0;
    }

    if (!(timbre->attack_seconds >= 0.0f &&
          timbre->attack_seconds <= 10.0f) ||
        !(timbre->decay_seconds >= 0.0f &&
          timbre->decay_seconds <= 10.0f) ||
        !(timbre->release_seconds >= 0.0f &&
          timbre->release_seconds <= 10.0f))
    {
        return 0;
    }

    if (!(timbre->sustain_level >= 0.0f &&
          timbre->sustain_level <= 1.0f) ||
        !(timbre->output_gain >= 0.0f &&
          timbre->output_gain <= 2.0f) ||
        !(timbre->velocity_brightness >= 0.0f &&
          timbre->velocity_brightness <= 1.0f))
    {
        return 0;
    }

    return 1;
}

static int fb_sfxMidiFmValidateTimbres(void)
{
    int program;

    for (program = 0; program < 128; ++program)
    {
        if (!fb_sfxMidiFmValidTimbre(
                &g_fb_sfx_midi_fm_timbres[program]))
        {
            SFX_DEBUG("sfx_midi_fm: invalid program preset %d", program);
            return -1;
        }
    }

    if (!fb_sfxMidiFmValidTimbre(&g_fb_sfx_midi_fm_drum_timbre))
    {
        SFX_DEBUG("sfx_midi_fm: invalid percussion preset");
        return -1;
    }

    return 0;
}

static void fb_sfxMidiFmBuildSineTable(void)
{
    int index;

    if (g_fb_sfx_midi_fm.sine_table_ready)
        return;

    for (index = 0; index < FB_SFX_MIDI_FM_SINE_SAMPLES; ++index)
    {
        double phase;

        phase = ((double)index * 6.28318530717958647692) /
                (double)FB_SFX_MIDI_FM_SINE_SAMPLES;
        g_fb_sfx_midi_fm.sine_table[index] = (float)sin(phase);
    }

    g_fb_sfx_midi_fm.sine_table_ready = 1;
}

static void fb_sfxMidiFmResetChannel(FB_SFX_MIDI_FM_CHANNEL *channel,
                                     int reset_program)
{
    if (!channel)
        return;

    if (reset_program)
        channel->program = 0;

    /*
        General MIDI controller defaults use 100 for channel volume and 127
        for expression. Pan 64 is the defined center value.
    */

    channel->volume = 100;
    channel->expression = 127;
    channel->pan = 64;
    channel->sustain = 0;
    channel->pitch_bend = 8192;
}

static void fb_sfxMidiFmResetState(void)
{
    int channel;

    memset(g_fb_sfx_midi_fm.voices, 0,
           sizeof(g_fb_sfx_midi_fm.voices));

    for (channel = 0; channel < FB_SFX_MIDI_FM_CHANNELS; ++channel)
        fb_sfxMidiFmResetChannel(&g_fb_sfx_midi_fm.channels[channel], 1);

    g_fb_sfx_midi_fm.paused = 0;
    g_fb_sfx_midi_fm.next_age = 1;
}


/* ------------------------------------------------------------------------- */
/* Oscillator helpers                                                        */
/* ------------------------------------------------------------------------- */

static float fb_sfxMidiFmWrapPhase(float phase)
{
    while (phase >= 1.0f)
        phase -= 1.0f;

    while (phase < 0.0f)
        phase += 1.0f;

    return phase;
}

static float fb_sfxMidiFmSine(float phase)
{
    float table_position;
    float fraction;
    int index;
    int next_index;

    phase = fb_sfxMidiFmWrapPhase(phase);
    table_position = phase * (float)FB_SFX_MIDI_FM_SINE_SAMPLES;
    index = (int)table_position;

    if (index >= FB_SFX_MIDI_FM_SINE_SAMPLES)
    {
        index = 0;
        fraction = 0.0f;
    }
    else
    {
        fraction = table_position - (float)index;
    }

    next_index = index + 1;
    if (next_index >= FB_SFX_MIDI_FM_SINE_SAMPLES)
        next_index = 0;

    return g_fb_sfx_midi_fm.sine_table[index] +
           (g_fb_sfx_midi_fm.sine_table[next_index] -
            g_fb_sfx_midi_fm.sine_table[index]) * fraction;
}

static float fb_sfxMidiFmNoise(FB_SFX_MIDI_FM_VOICE *voice)
{
    uint32_t value;

    if (!voice)
        return 0.0f;

    value = voice->noise_state;
    if (value == 0)
        value = 0x6D2B79F5u;

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    voice->noise_state = value;

    return ((float)(value & 0xFFFFu) / 32767.5f) - 1.0f;
}


/* ------------------------------------------------------------------------- */
/* Frequency and envelope helpers                                            */
/* ------------------------------------------------------------------------- */

static int fb_sfxMidiFmSampleRate(void)
{
    if (!__fb_sfx || __fb_sfx->samplerate <= 0)
        return FB_SFX_DEFAULT_RATE;

    return __fb_sfx->samplerate;
}

static float fb_sfxMidiFmNoteFrequency(int channel, int note)
{
    double bend;
    double note_number;
    double frequency;
    double maximum_frequency;
    int sample_rate;

    if (channel < 0 || channel >= FB_SFX_MIDI_FM_CHANNELS)
        return 440.0f;

    bend = ((double)g_fb_sfx_midi_fm.channels[channel].pitch_bend - 8192.0) *
           (2.0 / 8192.0);
    note_number = (double)note + bend;
    frequency = 440.0 * pow(2.0, (note_number - 69.0) / 12.0);

    sample_rate = fb_sfxMidiFmSampleRate();
    maximum_frequency = (double)sample_rate * 0.45;

    if (frequency < 20.0)
        frequency = 20.0;

    if (frequency > maximum_frequency)
        frequency = maximum_frequency;

    return (float)frequency;
}

static const FB_SFX_MIDI_FM_TIMBRE *fb_sfxMidiFmVoiceTimbre(
    const FB_SFX_MIDI_FM_VOICE *voice)
{
    if (!voice || voice->percussion)
        return &g_fb_sfx_midi_fm_drum_timbre;

    if (voice->program < 0 || voice->program > 127)
        return &g_fb_sfx_midi_fm_timbres[0];

    return &g_fb_sfx_midi_fm_timbres[voice->program];
}

static void fb_sfxMidiFmLimitModulation(FB_SFX_MIDI_FM_VOICE *voice)
{
    float highest_frequency;
    float modulation_bandwidth;
    float remaining_bandwidth;
    float ratio_scale;

    if (!voice)
        return;

    highest_frequency = (float)fb_sfxMidiFmSampleRate() * 0.45f;

    /*
        A preset which is clean in the middle of the keyboard can alias
        badly on a high note, especially at 11025 or 22050 Hz. Keep the
        first upper sideband below the guarded Nyquist limit, then reduce
        the modulation index when its useful sidebands would exceed the
        remaining bandwidth. The carrier remains audible as a clean sine
        when there is no room for additional brightness.
    */

    remaining_bandwidth = highest_frequency - voice->frequency;
    if (remaining_bandwidth <= 0.0f)
    {
        voice->modulation_index = 0.0f;
        return;
    }

    /*
        Keeping the modulator below Nyquist is not sufficient. An upper FM
        sideband appears at carrier + modulator, so the modulator is limited
        to the space above the carrier instead.
    */

    if (voice->frequency * voice->modulator_ratio > remaining_bandwidth)
    {
        ratio_scale = remaining_bandwidth /
                      (voice->frequency * voice->modulator_ratio);
        voice->modulator_ratio *= ratio_scale;
        voice->modulation_index *= ratio_scale;
    }

    modulation_bandwidth = voice->modulation_index *
                           voice->frequency *
                           voice->modulator_ratio;

    if (modulation_bandwidth > remaining_bandwidth)
    {
        voice->modulation_index *=
            remaining_bandwidth / modulation_bandwidth;
    }
}

static void fb_sfxMidiFmConfigureVoiceTimbre(
    FB_SFX_MIDI_FM_VOICE *voice,
    const FB_SFX_MIDI_FM_TIMBRE *timbre)
{
    float brightness;
    float velocity;

    if (!voice || !timbre)
        return;

    velocity = voice->velocity_gain * voice->velocity_gain;
    brightness = 1.0f - timbre->velocity_brightness +
                 timbre->velocity_brightness * velocity;

    voice->modulator_ratio = timbre->modulator_ratio;
    voice->modulation_index = timbre->modulation_index * brightness;
    voice->output_gain = timbre->output_gain;

    fb_sfxMidiFmLimitModulation(voice);
}

static void fb_sfxMidiFmReleaseVoice(FB_SFX_MIDI_FM_VOICE *voice)
{
    const FB_SFX_MIDI_FM_TIMBRE *timbre;
    float release_frames;

    if (!voice || !voice->active || voice->percussion)
        return;

    timbre = fb_sfxMidiFmVoiceTimbre(voice);
    release_frames = timbre->release_seconds *
                     (float)fb_sfxMidiFmSampleRate();

    if (release_frames < 1.0f)
        release_frames = 1.0f;

    voice->key_down = 0;
    voice->sustained = 0;
    voice->envelope_state = FB_SFX_MIDI_FM_RELEASE;
    voice->release_step = voice->envelope_level / release_frames;

    if (voice->release_step <= 0.0f)
        voice->release_step = 1.0f / release_frames;
}

static void fb_sfxMidiFmAdvanceEnvelope(FB_SFX_MIDI_FM_VOICE *voice,
                                        const FB_SFX_MIDI_FM_TIMBRE *timbre)
{
    float sample_rate;
    float step;

    if (!voice || !timbre)
        return;

    sample_rate = (float)fb_sfxMidiFmSampleRate();

    switch (voice->envelope_state)
    {
    case FB_SFX_MIDI_FM_ATTACK:
        if (timbre->attack_seconds <= 0.0f)
        {
            voice->envelope_level = 1.0f;
            voice->envelope_state = FB_SFX_MIDI_FM_DECAY;
            break;
        }

        voice->envelope_level += 1.0f /
                                 (timbre->attack_seconds * sample_rate);
        if (voice->envelope_level >= 1.0f)
        {
            voice->envelope_level = 1.0f;
            voice->envelope_state = FB_SFX_MIDI_FM_DECAY;
        }
        break;

    case FB_SFX_MIDI_FM_DECAY:
        if (timbre->decay_seconds <= 0.0f)
        {
            voice->envelope_level = timbre->sustain_level;
            voice->envelope_state = FB_SFX_MIDI_FM_SUSTAIN;
            break;
        }

        step = (1.0f - timbre->sustain_level) /
               (timbre->decay_seconds * sample_rate);
        voice->envelope_level -= step;

        if (voice->envelope_level <= timbre->sustain_level)
        {
            voice->envelope_level = timbre->sustain_level;
            voice->envelope_state = FB_SFX_MIDI_FM_SUSTAIN;
        }
        break;

    case FB_SFX_MIDI_FM_RELEASE:
        voice->envelope_level -= voice->release_step;
        if (voice->envelope_level <= 0.0f)
        {
            voice->envelope_level = 0.0f;
            voice->active = 0;
        }
        break;

    case FB_SFX_MIDI_FM_SUSTAIN:
    default:
        break;
    }
}


/* ------------------------------------------------------------------------- */
/* Voice allocation and note handling                                        */
/* ------------------------------------------------------------------------- */

static FB_SFX_MIDI_FM_VOICE *fb_sfxMidiFmAllocVoice(void)
{
    FB_SFX_MIDI_FM_VOICE *quietest_release;
    FB_SFX_MIDI_FM_VOICE *oldest;
    int index;

    quietest_release = NULL;
    oldest = NULL;

    for (index = 0; index < FB_SFX_MIDI_FM_VOICES; ++index)
    {
        FB_SFX_MIDI_FM_VOICE *voice;

        voice = &g_fb_sfx_midi_fm.voices[index];

        if (!voice->active)
            return voice;

        if (voice->envelope_state == FB_SFX_MIDI_FM_RELEASE &&
            (!quietest_release ||
             voice->envelope_level < quietest_release->envelope_level))
        {
            quietest_release = voice;
        }

        if (!oldest || voice->age < oldest->age)
            oldest = voice;
    }

    if (quietest_release)
        return quietest_release;

    return oldest;
}

static void fb_sfxMidiFmConfigureDrum(FB_SFX_MIDI_FM_VOICE *voice, int note)
{
    float duration;
    float frequency;
    float modulation_index;
    float modulator_ratio;
    float noise_mix;
    float output_gain;
    int sample_rate;

    if (!voice)
        return;

    duration = 0.14f;
    frequency = 900.0f;
    modulator_ratio = g_fb_sfx_midi_fm_drum_timbre.modulator_ratio;
    modulation_index = g_fb_sfx_midi_fm_drum_timbre.modulation_index;
    noise_mix = 0.60f;
    output_gain = g_fb_sfx_midi_fm_drum_timbre.output_gain;

    switch (note)
    {
    case 35:
        duration = 0.24f;
        frequency = 58.0f;
        modulator_ratio = 1.00f;
        modulation_index = 5.00f;
        noise_mix = 0.06f;
        output_gain = 0.82f;
        break;

    case 36:
        duration = 0.20f;
        frequency = 72.0f;
        modulator_ratio = 1.00f;
        modulation_index = 4.20f;
        noise_mix = 0.05f;
        output_gain = 0.84f;
        break;

    case 37:
        duration = 0.07f;
        frequency = 1150.0f;
        modulator_ratio = 3.00f;
        modulation_index = 1.60f;
        noise_mix = 0.32f;
        output_gain = 0.58f;
        break;

    case 38:
        duration = 0.18f;
        frequency = 190.0f;
        modulator_ratio = 1.50f;
        modulation_index = 1.20f;
        noise_mix = 0.82f;
        output_gain = 0.68f;
        break;

    case 39:
        duration = 0.13f;
        frequency = 1250.0f;
        modulator_ratio = 2.50f;
        modulation_index = 2.20f;
        noise_mix = 0.94f;
        output_gain = 0.58f;
        break;

    case 40:
        duration = 0.15f;
        frequency = 235.0f;
        modulator_ratio = 2.00f;
        modulation_index = 1.65f;
        noise_mix = 0.78f;
        output_gain = 0.65f;
        break;

    case 41:
    case 43:
    case 45:
    case 47:
    case 48:
    case 50:
        duration = 0.24f;
        frequency = 112.0f + (float)(note - 41) * 29.0f;
        modulator_ratio = 1.00f;
        modulation_index = 2.20f;
        noise_mix = 0.14f;
        output_gain = 0.72f;
        break;

    case 42:
        duration = 0.08f;
        frequency = 6200.0f;
        modulator_ratio = 5.00f;
        modulation_index = 3.40f;
        noise_mix = 0.92f;
        output_gain = 0.48f;
        break;

    case 44:
        duration = 0.10f;
        frequency = 5100.0f;
        modulator_ratio = 4.70f;
        modulation_index = 3.20f;
        noise_mix = 0.90f;
        output_gain = 0.47f;
        break;

    case 46:
        duration = 0.24f;
        frequency = 5400.0f;
        modulator_ratio = 5.00f;
        modulation_index = 3.80f;
        noise_mix = 0.90f;
        output_gain = 0.46f;
        break;

    case 49:
    case 52:
    case 55:
    case 57:
        duration = 0.58f;
        frequency = 2800.0f + (float)(note - 49) * 55.0f;
        modulator_ratio = 3.70f;
        modulation_index = 4.50f;
        noise_mix = 0.72f;
        output_gain = 0.48f;
        break;

    case 51:
    case 59:
        duration = 0.82f;
        frequency = 2200.0f + (float)(note - 51) * 35.0f;
        modulator_ratio = 3.20f;
        modulation_index = 3.80f;
        noise_mix = 0.64f;
        output_gain = 0.48f;
        break;

    case 53:
        duration = 0.42f;
        frequency = 1850.0f;
        modulator_ratio = 2.76f;
        modulation_index = 3.20f;
        noise_mix = 0.22f;
        output_gain = 0.54f;
        break;

    case 54:
        duration = 0.22f;
        frequency = 4100.0f;
        modulator_ratio = 4.00f;
        modulation_index = 3.20f;
        noise_mix = 0.84f;
        output_gain = 0.48f;
        break;

    case 56:
        duration = 0.28f;
        frequency = 540.0f;
        modulator_ratio = 1.50f;
        modulation_index = 2.30f;
        noise_mix = 0.08f;
        output_gain = 0.62f;
        break;

    case 58:
        duration = 0.48f;
        frequency = 760.0f;
        modulator_ratio = 3.00f;
        modulation_index = 3.60f;
        noise_mix = 0.66f;
        output_gain = 0.52f;
        break;

    case 60:
    case 61:
        duration = 0.19f;
        frequency = (note == 60) ? 480.0f : 360.0f;
        modulator_ratio = 1.00f;
        modulation_index = 1.85f;
        noise_mix = 0.08f;
        output_gain = 0.68f;
        break;

    case 62:
    case 63:
    case 64:
        duration = (note == 62) ? 0.15f : 0.27f;
        frequency = 420.0f - (float)(note - 62) * 70.0f;
        modulator_ratio = 1.00f;
        modulation_index = 2.10f;
        noise_mix = 0.12f;
        output_gain = 0.68f;
        break;

    case 65:
    case 66:
        duration = 0.22f;
        frequency = (note == 65) ? 620.0f : 480.0f;
        modulator_ratio = 1.50f;
        modulation_index = 2.40f;
        noise_mix = 0.12f;
        output_gain = 0.66f;
        break;

    case 67:
    case 68:
        duration = 0.30f;
        frequency = (note == 67) ? 980.0f : 760.0f;
        modulator_ratio = 1.50f;
        modulation_index = 1.80f;
        noise_mix = 0.04f;
        output_gain = 0.64f;
        break;

    case 69:
    case 70:
        duration = (note == 69) ? 0.13f : 0.08f;
        frequency = (note == 69) ? 4600.0f : 5600.0f;
        modulator_ratio = 4.50f;
        modulation_index = 3.80f;
        noise_mix = 0.96f;
        output_gain = 0.46f;
        break;

    case 71:
    case 72:
        duration = (note == 71) ? 0.18f : 0.52f;
        frequency = (note == 71) ? 2250.0f : 1800.0f;
        modulator_ratio = 1.00f;
        modulation_index = 0.48f;
        noise_mix = 0.06f;
        output_gain = 0.55f;
        break;

    case 73:
    case 74:
        duration = (note == 73) ? 0.16f : 0.42f;
        frequency = (note == 73) ? 1250.0f : 980.0f;
        modulator_ratio = 3.00f;
        modulation_index = 2.80f;
        noise_mix = 0.76f;
        output_gain = 0.50f;
        break;

    case 75:
        duration = 0.10f;
        frequency = 1420.0f;
        modulator_ratio = 2.00f;
        modulation_index = 1.10f;
        noise_mix = 0.04f;
        output_gain = 0.62f;
        break;

    case 76:
    case 77:
        duration = 0.12f;
        frequency = (note == 76) ? 950.0f : 720.0f;
        modulator_ratio = 2.00f;
        modulation_index = 1.45f;
        noise_mix = 0.03f;
        output_gain = 0.62f;
        break;

    case 78:
    case 79:
        duration = (note == 78) ? 0.18f : 0.42f;
        frequency = (note == 78) ? 520.0f : 390.0f;
        modulator_ratio = 0.50f;
        modulation_index = 3.10f;
        noise_mix = 0.08f;
        output_gain = 0.62f;
        break;

    case 80:
    case 81:
        duration = (note == 80) ? 0.28f : 1.00f;
        frequency = (note == 80) ? 3100.0f : 2700.0f;
        modulator_ratio = 2.01f;
        modulation_index = 1.80f;
        noise_mix = 0.02f;
        output_gain = 0.52f;
        break;

    default:
        break;
    }

    sample_rate = fb_sfxMidiFmSampleRate();

    if (frequency > (float)sample_rate * 0.45f)
        frequency = (float)sample_rate * 0.45f;

    voice->frequency = frequency;
    voice->modulator_ratio = modulator_ratio;
    voice->modulation_index = modulation_index *
                              (0.30f +
                               0.70f * voice->velocity_gain *
                               voice->velocity_gain);
    voice->noise_mix = noise_mix;
    voice->output_gain = output_gain;
    voice->drum_frames = (int)(duration * (float)sample_rate);

    fb_sfxMidiFmLimitModulation(voice);

    if (voice->drum_frames < 1)
        voice->drum_frames = 1;

    /*
        Decay to one thousandth of the starting level over the requested
        drum duration. This avoids a hard edge at the end of the voice.
    */

    voice->drum_decay = (float)pow(0.001,
                                    1.0 / (double)voice->drum_frames);
}

static void fb_sfxMidiFmNoteOn(int channel, int note, int velocity)
{
    const FB_SFX_MIDI_FM_TIMBRE *timbre;
    FB_SFX_MIDI_FM_VOICE *voice;
    int program;

    if (channel < 0 || channel >= FB_SFX_MIDI_FM_CHANNELS ||
        note < 0 || note > 127 || velocity <= 0 || velocity > 127)
    {
        return;
    }

    voice = fb_sfxMidiFmAllocVoice();
    if (!voice)
        return;

    memset(voice, 0, sizeof(*voice));

    program = g_fb_sfx_midi_fm.channels[channel].program;
    if (program < 0 || program > 127)
        program = 0;

    voice->active = 1;
    voice->key_down = 1;
    voice->channel = channel;
    voice->note = note;
    voice->program = program;
    voice->age = g_fb_sfx_midi_fm.next_age++;

    if (g_fb_sfx_midi_fm.next_age == 0)
        g_fb_sfx_midi_fm.next_age = 1;

    voice->noise_state = 0x9E3779B9u ^
                         (uint32_t)(voice->age * 2654435761u);
    voice->velocity_gain = (float)sqrt((double)velocity / 127.0);
    voice->envelope_state = FB_SFX_MIDI_FM_ATTACK;
    voice->frequency = fb_sfxMidiFmNoteFrequency(channel, note);
    timbre = &g_fb_sfx_midi_fm_timbres[program];
    fb_sfxMidiFmConfigureVoiceTimbre(voice, timbre);

    if (channel == FB_SFX_MIDI_FM_DRUM_CHANNEL)
    {
        voice->percussion = 1;
        voice->key_down = 0;
        voice->envelope_level = 1.0f;
        voice->envelope_state = FB_SFX_MIDI_FM_DECAY;
        fb_sfxMidiFmConfigureDrum(voice, note);
    }
}

static void fb_sfxMidiFmNoteOff(int channel, int note)
{
    FB_SFX_MIDI_FM_VOICE *oldest;
    int index;

    oldest = NULL;

    for (index = 0; index < FB_SFX_MIDI_FM_VOICES; ++index)
    {
        FB_SFX_MIDI_FM_VOICE *voice;

        voice = &g_fb_sfx_midi_fm.voices[index];

        if (!voice->active || voice->percussion || !voice->key_down)
            continue;

        if (voice->channel != channel || voice->note != note)
            continue;

        if (!oldest || voice->age < oldest->age)
            oldest = voice;
    }

    if (!oldest)
        return;

    oldest->key_down = 0;

    if (g_fb_sfx_midi_fm.channels[channel].sustain)
        oldest->sustained = 1;
    else
        fb_sfxMidiFmReleaseVoice(oldest);
}

static void fb_sfxMidiFmReleaseSustain(int channel)
{
    int index;

    for (index = 0; index < FB_SFX_MIDI_FM_VOICES; ++index)
    {
        FB_SFX_MIDI_FM_VOICE *voice;

        voice = &g_fb_sfx_midi_fm.voices[index];

        if (voice->active && voice->channel == channel &&
            voice->sustained && !voice->key_down)
        {
            fb_sfxMidiFmReleaseVoice(voice);
        }
    }
}

static void fb_sfxMidiFmReleaseChannel(int channel, int immediate)
{
    int index;

    for (index = 0; index < FB_SFX_MIDI_FM_VOICES; ++index)
    {
        FB_SFX_MIDI_FM_VOICE *voice;

        voice = &g_fb_sfx_midi_fm.voices[index];

        if (!voice->active || voice->channel != channel)
            continue;

        if (immediate)
            voice->active = 0;
        else if (!voice->percussion)
            fb_sfxMidiFmReleaseVoice(voice);
    }
}

static void fb_sfxMidiFmRetuneChannel(int channel)
{
    int index;

    for (index = 0; index < FB_SFX_MIDI_FM_VOICES; ++index)
    {
        FB_SFX_MIDI_FM_VOICE *voice;

        voice = &g_fb_sfx_midi_fm.voices[index];

        if (voice->active && !voice->percussion &&
            voice->channel == channel)
        {
            voice->frequency = fb_sfxMidiFmNoteFrequency(channel,
                                                         voice->note);
            fb_sfxMidiFmConfigureVoiceTimbre(
                voice, fb_sfxMidiFmVoiceTimbre(voice));
        }
    }
}


/* ------------------------------------------------------------------------- */
/* MIDI controller handling                                                  */
/* ------------------------------------------------------------------------- */

static void fb_sfxMidiFmControlChange(int channel, int controller, int value)
{
    FB_SFX_MIDI_FM_CHANNEL *state;

    if (channel < 0 || channel >= FB_SFX_MIDI_FM_CHANNELS)
        return;

    state = &g_fb_sfx_midi_fm.channels[channel];

    switch (controller)
    {
    case 7:
        state->volume = (unsigned char)value;
        break;

    case 10:
        state->pan = (unsigned char)value;
        break;

    case 11:
        state->expression = (unsigned char)value;
        break;

    case 64:
        if (value >= 64)
        {
            state->sustain = 1;
        }
        else
        {
            state->sustain = 0;
            fb_sfxMidiFmReleaseSustain(channel);
        }
        break;

    case 120:
        fb_sfxMidiFmReleaseChannel(channel, 1);
        break;

    case 121:
        if (state->sustain)
            fb_sfxMidiFmReleaseSustain(channel);
        fb_sfxMidiFmResetChannel(state, 0);
        fb_sfxMidiFmRetuneChannel(channel);
        break;

    case 123:
        fb_sfxMidiFmReleaseChannel(channel, 0);
        break;

    default:
        break;
    }
}


/* ------------------------------------------------------------------------- */
/* Software MIDI backend interface                                           */
/* ------------------------------------------------------------------------- */

int fb_sfxMidiSoftwareOpen(void)
{
    if (!__fb_sfx || __fb_sfx->samplerate <= 0)
        return -1;

    if (fb_sfxMidiFmValidateTimbres() != 0)
        return -1;

    fb_sfxMidiFmBuildSineTable();
    fb_sfxMidiFmResetState();
    g_fb_sfx_midi_fm.active = 1;

    SFX_DEBUG("sfx_midi_fm: opened %d-voice software MIDI synthesizer",
              FB_SFX_MIDI_FM_VOICES);
    return 0;
}

void fb_sfxMidiSoftwareClose(void)
{
    fb_sfxMidiFmResetState();
    g_fb_sfx_midi_fm.active = 0;

    SFX_DEBUG("sfx_midi_fm: closed");
}

void fb_sfxMidiSoftwareSilence(void)
{
    memset(g_fb_sfx_midi_fm.voices, 0,
           sizeof(g_fb_sfx_midi_fm.voices));
}

void fb_sfxMidiSoftwareReleaseAll(void)
{
    int channel;

    for (channel = 0; channel < FB_SFX_MIDI_FM_CHANNELS; ++channel)
        fb_sfxMidiFmReleaseChannel(channel, 0);
}

void fb_sfxMidiSoftwarePause(int paused)
{
    g_fb_sfx_midi_fm.paused = paused ? 1 : 0;
}

int fb_sfxMidiSoftwareSend(unsigned char status,
                           unsigned char data1,
                           unsigned char data2)
{
    int channel;
    int message;

    if (!g_fb_sfx_midi_fm.active)
        return -1;

    if (status < 0x80u)
        return -1;

    if (data1 > 127u || data2 > 127u)
        return -1;

    if (status >= 0xF0u)
    {
        if (status == 0xFFu)
            fb_sfxMidiFmResetState();

        return 0;
    }

    channel = status & 0x0F;
    message = status & 0xF0;

    switch (message)
    {
    case 0x80:
        fb_sfxMidiFmNoteOff(channel, data1);
        break;

    case 0x90:
        if (data2 == 0)
            fb_sfxMidiFmNoteOff(channel, data1);
        else
            fb_sfxMidiFmNoteOn(channel, data1, data2);
        break;

    case 0xB0:
        fb_sfxMidiFmControlChange(channel, data1, data2);
        break;

    case 0xC0:
        if (data1 <= 127)
            g_fb_sfx_midi_fm.channels[channel].program = data1;
        break;

    case 0xE0:
        g_fb_sfx_midi_fm.channels[channel].pitch_bend =
            (unsigned short)(((unsigned short)data2 << 7) |
                             (unsigned short)data1);
        fb_sfxMidiFmRetuneChannel(channel);
        break;

    default:
        /*
            Polyphonic pressure and channel pressure are legal MIDI channel
            messages but do not affect this minimal synthesizer.
        */
        break;
    }

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Mixer integration                                                         */
/* ------------------------------------------------------------------------- */

static float fb_sfxMidiFmRenderVoice(FB_SFX_MIDI_FM_VOICE *voice)
{
    const FB_SFX_MIDI_FM_TIMBRE *timbre;
    float carrier_increment;
    float modulator_increment;
    float modulation;
    float tonal_sample;
    float sample;
    float sample_rate;

    if (!voice || !voice->active)
        return 0.0f;

    timbre = fb_sfxMidiFmVoiceTimbre(voice);
    sample_rate = (float)fb_sfxMidiFmSampleRate();
    carrier_increment = voice->frequency / sample_rate;
    modulator_increment = carrier_increment * voice->modulator_ratio;

    modulation = fb_sfxMidiFmSine(voice->modulator_phase) *
                 voice->modulation_index *
                 voice->envelope_level;

    tonal_sample = fb_sfxMidiFmSine(
        voice->carrier_phase + modulation / FB_SFX_MIDI_FM_TWO_PI);
    sample = tonal_sample;

    if (voice->percussion && voice->noise_mix > 0.0f)
    {
        sample = tonal_sample * (1.0f - voice->noise_mix) +
                 fb_sfxMidiFmNoise(voice) * voice->noise_mix;
    }

    voice->carrier_phase = fb_sfxMidiFmWrapPhase(
        voice->carrier_phase + carrier_increment);
    voice->modulator_phase = fb_sfxMidiFmWrapPhase(
        voice->modulator_phase + modulator_increment);

    if (voice->percussion)
    {
        voice->envelope_level *= voice->drum_decay;
        voice->drum_frames--;

        if (voice->drum_frames <= 0 || voice->envelope_level < 0.0005f)
            voice->active = 0;
    }
    else
    {
        fb_sfxMidiFmAdvanceEnvelope(voice, timbre);
    }

    return sample *
           voice->envelope_level *
           voice->velocity_gain *
           voice->output_gain;
}

void fb_sfxMidiSoftwareMixFrame(float *left, float *right)
{
    float synth_left;
    float synth_right;
    float normalizer;
    float master_volume;
    float balance;
    int active_voices;
    int index;

    if (!left || !right || !g_fb_sfx_midi_fm.active ||
        g_fb_sfx_midi_fm.paused)
    {
        return;
    }

    synth_left = 0.0f;
    synth_right = 0.0f;
    active_voices = 0;
    master_volume = (__fb_sfx) ? __fb_sfx->master_volume : 1.0f;
    balance = (__fb_sfx) ? __fb_sfx->balance : 0.0f;

    for (index = 0; index < FB_SFX_MIDI_FM_VOICES; ++index)
    {
        FB_SFX_MIDI_FM_CHANNEL *channel;
        FB_SFX_MIDI_FM_VOICE *voice;
        float channel_gain;
        float pan;
        float sample;

        voice = &g_fb_sfx_midi_fm.voices[index];
        if (!voice->active)
            continue;

        channel = &g_fb_sfx_midi_fm.channels[voice->channel];
        sample = fb_sfxMidiFmRenderVoice(voice);
        active_voices++;

        channel_gain = ((float)channel->volume / 127.0f) *
                       ((float)channel->expression / 127.0f);
        sample *= channel_gain;

        pan = ((float)channel->pan - 64.0f) / 63.0f;
        pan += balance;

        if (pan < -1.0f)
            pan = -1.0f;
        if (pan > 1.0f)
            pan = 1.0f;

        synth_left += sample * (1.0f - pan) * 0.5f;
        synth_right += sample * (1.0f + pan) * 0.5f;
    }

    if (active_voices <= 0)
        return;

    /*
        Square-root normalization keeps a solo voice audible while leaving
        useful headroom for ordinary chords. The final mixer clamp remains
        the last line of defense for unusually dense files.
    */

    normalizer = 0.34f / (float)sqrt((double)active_voices);
    *left += synth_left * normalizer * master_volume;
    *right += synth_right * normalizer * master_volume;
}

/* end of sfx_midi_fm.c */
