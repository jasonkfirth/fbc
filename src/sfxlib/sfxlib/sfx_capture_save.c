/*
    FreeBASIC Sound Library (sfxlib)
    --------------------------------

    File: sfx_capture_save.c

    Purpose:

        Implement the CAPTURE SAVE command.

        This command writes captured audio samples to a file.
        The initial implementation writes a simple 16-bit PCM
        WAV file so the output can be easily inspected or used
        by other tools.

    Responsibilities:

        • receive captured samples
        • convert float samples to 16-bit PCM
        • write a valid WAV file
        • perform defensive validation

    This file intentionally does NOT contain:

        • capture device implementation
        • mixer logic
        • streaming playback logic

    Architectural overview:

        capture device
              │
        capture subsystem
              │
        sfx_capture_save
              │
        WAV file output
*/

#include "fb_sfx.h"
#include "fb_sfx_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* External capture interface                                                */
/* ------------------------------------------------------------------------- */

extern int fb_sfxCaptureReadSamples(float *buffer, int frames);


/* ------------------------------------------------------------------------- */
/* WAV header structure                                                      */
/* ------------------------------------------------------------------------- */

typedef struct WAV_HEADER
{
    char  riff[4];
    unsigned int file_size;
    char  wave[4];

    char  fmt[4];
    unsigned int fmt_size;
    unsigned short format;
    unsigned short channels;
    unsigned int sample_rate;
    unsigned int byte_rate;
    unsigned short block_align;
    unsigned short bits_per_sample;

    char  data[4];
    unsigned int data_size;

} WAV_HEADER;


/* ------------------------------------------------------------------------- */
/* Float → PCM conversion                                                    */
/* ------------------------------------------------------------------------- */

static short fb_sfxFloatToPCM16(float v)
{
    if (v > 1.0f)
        v = 1.0f;

    if (v < -1.0f)
        v = -1.0f;

    return (short)(v * 32767.0f);
}


/* ------------------------------------------------------------------------- */
/* CAPTURE SAVE                                                              */
/* ------------------------------------------------------------------------- */

int fb_sfxCaptureSave(const char *filename, int seconds)
{
    FILE *f;
    int sample_rate = 44100;
    int channels = 2;
    int frames;
    int total_frames;
    int i;

    float *buffer;
    short *pcm;

    WAV_HEADER header;

    if (!filename)
        return -1;

    if (seconds <= 0)
        return -1;

    f = fopen(filename, "wb");

    if (!f)
    {
        SFX_DEBUG("sfx_capture_save: failed to open file");
        return -1;
    }

    total_frames = sample_rate * seconds;

    buffer = (float*)malloc(sizeof(float) * total_frames * channels);
    pcm    = (short*)malloc(sizeof(short) * total_frames * channels);

    if (!buffer || !pcm)
    {
        fclose(f);
        free(buffer);
        free(pcm);
        return -1;
    }

    /*
        Read captured samples
    */

    frames = fb_sfxCaptureReadSamples(buffer, total_frames);

    if (frames <= 0)
    {
        fclose(f);
        free(buffer);
        free(pcm);
        return -1;
    }

    /*
        Convert to PCM
    */

    for (i = 0; i < frames * channels; i++)
        pcm[i] = fb_sfxFloatToPCM16(buffer[i]);

    /*
        Prepare WAV header
    */

    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt,  "fmt ", 4);
    memcpy(header.data, "data", 4);

    header.fmt_size = 16;
    header.format = 1;
    header.channels = channels;
    header.sample_rate = sample_rate;
    header.bits_per_sample = 16;

    header.block_align = channels * (header.bits_per_sample / 8);
    header.byte_rate = header.sample_rate * header.block_align;

    header.data_size = frames * header.block_align;
    header.file_size = 36 + header.data_size;

    /*
        Write file
    */

    fwrite(&header, sizeof(header), 1, f);
    fwrite(pcm, header.data_size, 1, f);

    fclose(f);

    free(buffer);
    free(pcm);

    return 0;
}


/* end of sfx_capture_save.c */
