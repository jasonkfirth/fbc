/*
    FreeBASIC FMOD 3.75 native qualification
    File: fmod-native.c
    Purpose: Inspect the vendor ABI and exercise native module and stream output.
    Responsibilities: Guarded tag outputs, bounded playback and owned resources.
    This file contains no replacement decoder or audio driver implementation.
*/
#include <fmod.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static void wait_for_output(void)
{
#ifdef _WIN32
    Sleep(1200);
#else
    struct timespec delay = {1, 200000000};
    nanosleep(&delay, NULL);
#endif
}

int main(int argc, char **argv)
{
    FMUSIC_MODULE *module = NULL;
    FSOUND_STREAM *stream = NULL;
    struct { uint32_t before; int count; uint32_t after; } tags = {0xA1B2C3D4, 0, 0xE5F60718};
    int result = 1, audio_channel;
    printf("ABI %u %u %u %u %u %u %u %u %u %u %u %u\n",
           (unsigned)sizeof(void *), (unsigned)sizeof(enum FMOD_ERRORS),
           (unsigned)sizeof(enum FSOUND_OUTPUTTYPES), (unsigned)sizeof(FSOUND_REVERB_PROPERTIES),
           (unsigned)offsetof(FSOUND_REVERB_PROPERTIES, ReflectionsPan),
           (unsigned)offsetof(FSOUND_REVERB_PROPERTIES, Flags),
           (unsigned)sizeof(FSOUND_REVERB_CHANNELPROPERTIES),
           (unsigned)offsetof(FSOUND_REVERB_CHANNELPROPERTIES, Flags),
           (unsigned)sizeof(FSOUND_TOC_TAG), (unsigned)offsetof(FSOUND_TOC_TAG, frame),
           (unsigned)FSOUND_OUTPUT_ALSA, (unsigned)FSOUND_TAGFIELD_ID3V2);
    printf("VERSION %.2f\n", FSOUND_GetVersion());
    if(argc == 1) return 0;
#ifndef _WIN32
    /* Use the Linux 3.75 SDK's default OSS route with a real compatible
       character device. This does not select either no-sound output mode. */
    if(!FSOUND_SetOutput(FSOUND_OUTPUT_OSS)) return 2;
#endif
    if(!FSOUND_Init(44100, 32, 0)) {
        printf("INIT_ERROR=%d\n", FSOUND_GetError());
        return 2;
    }
    if(argc != 3) goto cleanup;
    if(strcmp(argv[1], "module") == 0) {
        module = FMUSIC_LoadSong(argv[2]);
        if(!module || !FMUSIC_PlaySong(module)) goto cleanup;
        wait_for_output();
        if(!FMUSIC_IsPlaying(module) || FMUSIC_GetRow(module) <= 0) goto cleanup;
        if(FMUSIC_GetTime(module) < 500 || FMUSIC_GetTime(module) > 3000) goto cleanup;
        printf("FMOD_C_MODULE=PASS row=%d order=%d time=%d\n", FMUSIC_GetRow(module), FMUSIC_GetOrder(module), FMUSIC_GetTime(module));
        if(!FMUSIC_StopSong(module)) goto cleanup;
    }
    else {
        int length = 0;
        void *value = NULL;
        stream = FSOUND_Stream_Open(argv[2], FSOUND_MPEGACCURATE | FSOUND_LOOP_NORMAL, 0, 0);
        if(!stream) goto cleanup;
        if(!FSOUND_Stream_GetNumTagFields(stream, &tags.count)) goto cleanup;
        if(tags.before != 0xA1B2C3D4 || tags.after != 0xE5F60718 || tags.count < 0 || tags.count > 1024) goto cleanup;
        if(FSOUND_Stream_FindTagField(stream, FSOUND_TAGFIELD_ID3V2, "TIT2", &value, &length)) {
            if(!value || length <= 0 || length > 4096) goto cleanup;
            printf("TITLE_BYTES=%d\n", length);
        }
        audio_channel = FSOUND_Stream_Play(FSOUND_FREE, stream);
        if(audio_channel < 0) goto cleanup;
        wait_for_output();
        if(!FSOUND_IsPlaying(audio_channel) || FSOUND_Stream_GetPosition(stream) == 0) goto cleanup;
        printf("FMOD_C_STREAM=PASS bytes=%u tags=%d\n", FSOUND_Stream_GetPosition(stream), tags.count);
        if(!FSOUND_Stream_Stop(stream)) goto cleanup;
    }
    result = 0;
cleanup:
    if(result != 0) printf("RUNTIME_ERROR=%d\n", FSOUND_GetError());
    if(module && !FMUSIC_FreeSong(module)) result = 1;
    if(stream && !FSOUND_Stream_Close(stream)) result = 1;
    FSOUND_Close();
    return result;
}
/* end of fmod-native.c */
