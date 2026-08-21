/*
    FreeBASIC Sound Library support for Windows CE
    -----------------------------------------------

    File: sfx_decode.c

    Purpose:

        Decode audio files into floating-point PCM samples on Windows CE.

    Responsibilities:

        - avoid CeGCC's incomplete desktop-compatible errno layer
        - dispatch supported file extensions to bundled decoders
        - load source files through the Windows CE C runtime
        - copy decoder-owned buffers into sfxlib-owned memory
        - normalize integer decoded samples into mixer-ready floats

    This file intentionally does NOT contain:

        - playback scheduling
        - mixer state
        - WinMM output implementation
        - changes to shared third-party source drops
*/

#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#define STB_VORBIS_NO_STDIO

/*
    stb_vorbis uses alloca() internally but not every GCC target SDK exposes
    the non-standard declaration through a public header.  Use the compiler's
    architecture-neutral intrinsic at this third-party boundary.  This keeps
    the decoder portable without placing a Windows CE conditional in shared
    code.
*/
#if defined(__GNUC__) && !defined(alloca)
#define alloca __builtin_alloca
#endif

/*
    Bundled decoder boundary

    dr_wav, dr_mp3, and stb_vorbis are maintained as upstream source drops.
    Their implementation warnings are not actionable sfxlib diagnostics, so
    keep strict warning policy outside this boundary while still compiling the
    decoder code normally.
*/

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wlogical-op"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wshadow=compatible-local"
#pragma GCC diagnostic ignored "-Wshadow=local"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include "../third_party/dr_wav.h"
#include "../third_party/dr_mp3.h"

/*
    Analyze the sfxlib-owned call boundary without reinterpreting stb_vorbis
    internals.  The analyzer otherwise explores an impossible mix of the
    library's push and pull modes and reports a null output path that
    stb_vorbis_decode_filename cannot enter.
*/
#if defined(__clang_analyzer__)
#define STB_VORBIS_HEADER_ONLY
#endif
#include "../third_party/stb_vorbis.c"
#if defined(__clang_analyzer__)
#undef STB_VORBIS_HEADER_ONLY
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../fb_sfx.h"
#include "../fb_sfx_internal.h"

/* ------------------------------------------------------------------------- */
/* File loading                                                              */
/* ------------------------------------------------------------------------- */

/*
    CeGCC's errno.h forwards to a header that is absent from the maintained
    ARM toolchain.  Keep third-party stdio adapters disabled and feed each
    decoder an in-memory image loaded through the C runtime instead.
*/
static int fb_sfxLoadFile(const char *filename,
                          unsigned char **data,
                          size_t *data_size)
{
    unsigned char *buffer;
    FILE *file;
    long file_size;
    size_t bytes_read;

    if (!filename || !data || !data_size)
        return -1;

    *data = NULL;
    *data_size = 0;

    file = fb_sfxOpenFile(filename, "rb");
    if (!file)
        return -1;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return -1;
    }

    file_size = ftell(file);
    if (file_size <= 0 || fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return -1;
    }

    buffer = (unsigned char *)malloc((size_t)file_size);
    if (!buffer)
    {
        fclose(file);
        return -1;
    }

    bytes_read = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size)
    {
        free(buffer);
        return -1;
    }

    *data = buffer;
    *data_size = bytes_read;
    return 0;
}

static const char *fb_sfxFileExt(const char *filename)
{
    const char *dot = NULL;

    if (!filename)
        return NULL;

    while (*filename)
    {
        if (*filename == '.')
            dot = filename;
        filename++;
    }

    return dot;
}

static int fb_sfxExtEquals(const char *ext, const char *want)
{
    while (*ext && *want)
    {
        if (tolower((unsigned char)*ext) != tolower((unsigned char)*want))
            return 0;
        ext++;
        want++;
    }

    return (*ext == '\0' && *want == '\0');
}

static int fb_sfxDecodeWav(const char *filename,
                           float **samples,
                           int *frames,
                           int *channels,
                           int *sample_rate)
{
    unsigned char *file_data;
    size_t file_size;
    unsigned int local_channels = 0;
    unsigned int local_rate = 0;
    drwav_uint64 frame_count = 0;
    float *decoded;
    size_t sample_count;
    float *copy;

    if (fb_sfxLoadFile(filename, &file_data, &file_size) != 0)
        return -1;

    decoded = drwav_open_memory_and_read_pcm_frames_f32(file_data,
                                                        file_size,
                                                        &local_channels,
                                                        &local_rate,
                                                        &frame_count,
                                                        NULL);
    free(file_data);
    if (!decoded || frame_count == 0 || local_channels == 0)
    {
        if (decoded)
            drwav_free(decoded, NULL);
        return -1;
    }

    sample_count = (size_t)frame_count * (size_t)local_channels;
    copy = (float *)malloc(sample_count * sizeof(float));
    if (!copy)
    {
        drwav_free(decoded, NULL);
        return -1;
    }

    memcpy(copy, decoded, sample_count * sizeof(float));
    drwav_free(decoded, NULL);

    *samples = copy;
    *frames = (int)frame_count;
    *channels = (int)local_channels;
    *sample_rate = (int)local_rate;
    return 0;
}

static int fb_sfxDecodeMp3(const char *filename,
                           float **samples,
                           int *frames,
                           int *channels,
                           int *sample_rate)
{
    unsigned char *file_data;
    size_t file_size;
    drmp3_config config;
    drmp3_uint64 frame_count = 0;
    float *decoded;
    size_t sample_count;
    float *copy;

    if (fb_sfxLoadFile(filename, &file_data, &file_size) != 0)
        return -1;

    decoded = drmp3_open_memory_and_read_pcm_frames_f32(file_data,
                                                        file_size,
                                                        &config,
                                                        &frame_count,
                                                        NULL);
    free(file_data);
    if (!decoded || frame_count == 0 || config.channels == 0)
    {
        if (decoded)
            drmp3_free(decoded, NULL);
        return -1;
    }

    sample_count = (size_t)frame_count * (size_t)config.channels;
    copy = (float *)malloc(sample_count * sizeof(float));
    if (!copy)
    {
        drmp3_free(decoded, NULL);
        return -1;
    }

    memcpy(copy, decoded, sample_count * sizeof(float));
    drmp3_free(decoded, NULL);

    *samples = copy;
    *frames = (int)frame_count;
    *channels = (int)config.channels;
    *sample_rate = (int)config.sampleRate;
    return 0;
}

static int fb_sfxDecodeOgg(const char *filename,
                           float **samples,
                           int *frames,
                           int *channels,
                           int *sample_rate)
{
    unsigned char *file_data;
    size_t file_size;
    short *decoded = NULL;
    int local_channels = 0;
    int local_rate = 0;
    int frame_count;
    size_t sample_count;
    float *copy;

    if (fb_sfxLoadFile(filename, &file_data, &file_size) != 0)
        return -1;

    if (file_size > (size_t)INT_MAX)
    {
        free(file_data);
        return -1;
    }

    frame_count = stb_vorbis_decode_memory(file_data,
                                           (int)file_size,
                                           &local_channels,
                                           &local_rate,
                                           &decoded);
    free(file_data);
    if (frame_count <= 0 || local_channels <= 0 || !decoded)
    {
        free(decoded);
        return -1;
    }

    sample_count = (size_t)frame_count * (size_t)local_channels;
    if (sample_count > (size_t)INT_MAX)
    {
        free(decoded);
        return -1;
    }

    copy = (float *)malloc(sample_count * sizeof(float));
    if (!copy)
    {
        free(decoded);
        return -1;
    }

    fb_sfxConvertS16ToFloat(decoded, copy, (int)sample_count);

    free(decoded);

    *samples = copy;
    *frames = frame_count;
    *channels = local_channels;
    *sample_rate = local_rate;
    return 0;
}

int fb_sfxDecodeFile(const char *filename,
                     float **samples,
                     int *frames,
                     int *channels,
                     int *sample_rate)
{
    const char *ext;

    if (!filename || !samples || !frames || !channels || !sample_rate)
        return -1;

    *samples = NULL;
    *frames = 0;
    *channels = 0;
    *sample_rate = 0;

    ext = fb_sfxFileExt(filename);
    if (!ext)
        return -1;

    if (fb_sfxExtEquals(ext, ".wav"))
        return fb_sfxDecodeWav(filename, samples, frames, channels, sample_rate);

    if (fb_sfxExtEquals(ext, ".mp3"))
        return fb_sfxDecodeMp3(filename, samples, frames, channels, sample_rate);

    if (fb_sfxExtEquals(ext, ".ogg"))
        return fb_sfxDecodeOgg(filename, samples, frames, channels, sample_rate);

    return -1;
}

/* end of sfx_decode.c */
