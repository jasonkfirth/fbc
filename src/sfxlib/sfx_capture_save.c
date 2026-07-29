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

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* ------------------------------------------------------------------------- */
/* WAV header structure                                                      */
/* ------------------------------------------------------------------------- */

typedef struct WAV_HEADER
{
    char  riff[4];
    uint32_t file_size;
    char  wave[4];

    char  fmt[4];
    uint32_t fmt_size;
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;

    char  data[4];
    uint32_t data_size;

} WAV_HEADER;


/* ------------------------------------------------------------------------- */
/* CAPTURE SAVE                                                              */
/* ------------------------------------------------------------------------- */

int fb_sfxCaptureSave(const char *filename, int seconds)
{
    FILE *f;
    const int sample_rate = 44100;
    const int channels = 2;
    int frames;
    int total_frames;
    int result = -1;

    float *buffer = NULL;
    short *pcm = NULL;
    size_t sample_count;
    size_t buffer_bytes;
    size_t pcm_bytes;

    WAV_HEADER header;

    if (!filename || seconds <= 0)
        return -1;

    if (seconds > INT_MAX / sample_rate)
        return -1;

    total_frames = sample_rate * seconds;
    if (total_frames > INT_MAX / channels)
        return -1;

    sample_count = (size_t)total_frames * (size_t)channels;
    if (sample_count > SIZE_MAX / sizeof(*buffer) ||
        sample_count > SIZE_MAX / sizeof(*pcm))
        return -1;

    buffer_bytes = sample_count * sizeof(*buffer);
    pcm_bytes = sample_count * sizeof(*pcm);

    f = fopen(filename, "wb");
    if (!f)
    {
        SFX_DEBUG("sfx_capture_save: failed to open file");
        return -1;
    }

    buffer = (float*)malloc(buffer_bytes);
    pcm = (short*)malloc(pcm_bytes);

    if (!buffer || !pcm)
        goto done;

    /*
        Read captured samples
    */

    frames = fb_sfxCaptureReadSamples(buffer, total_frames);

    if (frames <= 0 || frames > total_frames)
        goto done;

    /*
        Convert to PCM
    */

    fb_sfxConvertFloatToS16(buffer, pcm, frames * channels);

    /*
        Prepare WAV header
    */

    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt,  "fmt ", 4);
    memcpy(header.data, "data", 4);

    header.fmt_size = UINT32_C(16);
    header.format = UINT16_C(1);
    header.channels = (uint16_t)(unsigned int)channels;
    header.sample_rate = (uint32_t)(unsigned int)sample_rate;
    header.bits_per_sample = UINT16_C(16);

    header.block_align = (uint16_t)((unsigned int)channels *
        ((unsigned int)header.bits_per_sample / 8u));
    header.byte_rate = header.sample_rate * (uint32_t)header.block_align;

    if ((uint32_t)frames >
        (UINT32_MAX - UINT32_C(36)) / (uint32_t)header.block_align)
        goto done;

    header.data_size = (uint32_t)frames * (uint32_t)header.block_align;
    header.file_size = UINT32_C(36) + header.data_size;

    /*
        Write file
    */

    if (fwrite(&header, sizeof(header), 1, f) != 1 ||
        fwrite(pcm, 1, header.data_size, f) != header.data_size)
        goto done;

    result = 0;

done:
    fclose(f);

    free(buffer);
    free(pcm);

    return result;
}


/* end of sfx_capture_save.c */
