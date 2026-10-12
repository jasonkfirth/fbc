/*
    FreeBASIC native BASS qualification
    File: bass-decode.c
    Purpose: Produce independent native PCM bytes for stream and module checks.
    Responsibilities: Bounded reads, buffer guards and handle/file cleanup.
    This file contains no playback or alternative audio decoder.
*/
#include <bass.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct guarded_audio { uint32_t before; float samples[4096]; uint32_t after; };

int main(int argc, char **argv)
{
    struct guarded_audio audio = {0xA1B2C3D4, {0}, 0xE5F60718};
    FILE *output = NULL;
    DWORD channel = 0, total = 0, received;
    int module, result = 1;
    /* Four MiB bounds the partial module reference without decoding an
       entire song. Stream cases must reach end of file within this budget. */
    const DWORD limit = 4 * 1024 * 1024;
    if(argc != 4) return 1;
    module = strcmp(argv[3], "music") == 0;
    if(!BASS_Init(0, 44100, 0, NULL, NULL)) return 2;
    if(module)
        channel = BASS_MusicLoad(FALSE, argv[1], 0, 0,
                                BASS_MUSIC_DECODE | BASS_SAMPLE_FLOAT | BASS_MUSIC_PRESCAN, 0);
    else
        channel = BASS_StreamCreateFile(FALSE, argv[1], 0, 0, BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT);
    if(!channel) goto cleanup;
    output = fopen(argv[2], "wb");
    if(!output) goto cleanup;
    while(total < limit) {
        received = BASS_ChannelGetData(channel, audio.samples, sizeof(audio.samples));
        if(audio.before != 0xA1B2C3D4 || audio.after != 0xE5F60718) goto cleanup;
        if(received == (DWORD)-1) {
            if(BASS_ErrorGetCode() != BASS_ERROR_ENDED) goto cleanup;
            break;
        }
        if(received == 0 || received > sizeof(audio.samples) || received % sizeof(float) != 0) goto cleanup;
        if(fwrite(audio.samples, 1, received, output) != received) goto cleanup;
        total += received;
    }
    if(total == 0 || (!module && total >= limit)) goto cleanup;
    result = 0;
cleanup:
    if(output && fclose(output) != 0) result = 1;
    if(channel) {
        if(module) {
            if(!BASS_MusicFree(channel)) result = 1;
        }
        else if(!BASS_StreamFree(channel)) result = 1;
    }
    if(!BASS_Free()) result = 1;
    if(result == 0) printf("BASS_C_DECODE=PASS bytes=%u\n", (unsigned)total);
    return result;
}
/* end of bass-decode.c */
