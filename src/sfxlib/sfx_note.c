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
} FB_SFX_NOTEENTRY;


static const FB_SFX_NOTEENTRY note_table[] =
{
    {"C", 261},
    {"C#",277},
    {"D", 293},
    {"D#",311},
    {"E", 329},
    {"F", 349},
    {"F#",370},
    {"G", 392},
    {"G#",415},
    {"A", 440},
    {"A#",466},
    {"B", 493}
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
    int shift;

    if (base <= 0)
        return 0;

    shift = octave - 4;

    while (shift > 0)
    {
        base *= 2;
        shift--;
    }

    while (shift < 0)
    {
        base /= 2;
        shift++;
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
        duration,
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
        duration,
        freq
    );
}


/* end of sfx_note.c */
