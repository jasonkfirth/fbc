/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_note.c

    Purpose:

        Implement the BASIC NOTE command.

        NOTE provides a musical interface for generating tones
        using note names and octaves rather than raw frequency
        values.

    Responsibilities:

        • convert musical notes to oscillator frequencies
        • allocate voices for note playback
        • apply duration and channel settings

    This file intentionally does NOT contain:

        • oscillator algorithms
        • envelope processing
        • mixer logic
        • driver interaction

    Architectural overview:

        NOTE command
             │
             ▼
        note→frequency conversion
             │
             ▼
        voice allocation
             │
             ▼
        oscillator → envelope → mixer → buffer → driver
*/

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "fb_sfx.h"
#include "fb_sfx_internal.h"


/* ------------------------------------------------------------------------- */
/* Note table (base frequencies for octave 4)                                */
/* ------------------------------------------------------------------------- */

typedef struct
{
    const char *name;
    int frequency;
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
    /* Name the existing 64-bit tail padding without changing the layout. */
    unsigned char tail_padding[4];
#endif
} FB_SFX_NOTEENTRY;


static const FB_SFX_NOTEENTRY note_table[] =
{
    {.name = "C",  .frequency = 261},
    {.name = "C#", .frequency = 277},
    {.name = "D",  .frequency = 293},
    {.name = "D#", .frequency = 311},
    {.name = "E",  .frequency = 329},
    {.name = "F",  .frequency = 349},
    {.name = "F#", .frequency = 370},
    {.name = "G",  .frequency = 392},
    {.name = "G#", .frequency = 415},
    {.name = "A",  .frequency = 440},
    {.name = "A#", .frequency = 466},
    {.name = "B",  .frequency = 493}
};


/* ------------------------------------------------------------------------- */
/* Note lookup                                                               */
/* ------------------------------------------------------------------------- */

static int fb_sfxNoteLookup(const char *note)
{
    int i;

    if (!note)
        return 0;

    for (i = 0; i < (int)(sizeof(note_table)/sizeof(note_table[0])); i++)
    {
        if (strcmp(note, note_table[i].name) == 0)
            return note_table[i].frequency;
    }

    return 0;
}


/* ------------------------------------------------------------------------- */
/* Octave adjustment                                                         */
/* ------------------------------------------------------------------------- */

static int fb_sfxNoteApplyOctave(int base, int octave)
{
    int64_t shift;
	uint64_t steps;

    if (base <= 0)
        return 0;

    shift = (int64_t)octave - 4;

	if (shift > 0) {
		steps = (uint64_t)shift;
		while (steps > 0u) {
			if (base > INT_MAX / 2)
				return 0;
			base *= 2;
			steps--;
		}
	} else if (shift < 0) {
		/* The widened octave difference cannot be INT64_MIN. */
		steps = 0u - (uint64_t)shift;
		while ((steps > 0u) && (base > 0)) {
			base /= 2;
			steps--;
		}
	}

    return base;
}


/* ------------------------------------------------------------------------- */
/* NOTE command                                                              */
/* ------------------------------------------------------------------------- */

/*
    fb_sfxNote()

    Generate a musical note using the internal synthesis system.

    Parameters:

        note      musical note string ("C", "F#", etc)
        octave    octave number
        duration  length of note in seconds
*/

void fb_sfxNote(const char *note, int octave, float duration)
{
    int freq;

    if (!fb_sfxEnsureInitialized())
        return;

    if (!note)
        return;

    if (duration <= 0.0f)
        return;

    freq = fb_sfxNoteLookup(note);

    if (freq <= 0)
        return;

    freq = fb_sfxNoteApplyOctave(freq, octave);

    fb_sfxTone(0, freq, duration);

    SFX_DEBUG(
        "sfx_note: note=%s octave=%d duration=%f freq=%d",
        note,
        octave,
        (double)duration,
        freq
    );
}


/* ------------------------------------------------------------------------- */
/* Channel NOTE                                                              */
/* ------------------------------------------------------------------------- */

void fb_sfxNoteChannel(
    int channel,
    const char *note,
    int octave,
    float duration)
{
    int freq;

    if (!fb_sfxEnsureInitialized())
        return;

    if (!note)
        return;

    freq = fb_sfxNoteLookup(note);

    if (freq <= 0)
        return;

    freq = fb_sfxNoteApplyOctave(freq, octave);

    fb_sfxTone(channel, freq, duration);

    SFX_DEBUG(
        "sfx_note: channel=%d note=%s octave=%d duration=%f freq=%d",
        channel,
        note,
        octave,
        (double)duration,
        freq
    );
}


/* end of sfx_note.c */
