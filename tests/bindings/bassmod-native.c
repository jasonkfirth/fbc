/*
    FreeBASIC BASSMOD native qualification
    File: bassmod-native.c
    Purpose: Inspect the vendor ABI, then exercise native output and decoding.
    Responsibilities: Bounded PCM buffers, resource cleanup and error statuses.
    This file contains no substitute module engine or OSS implementation.
*/
#include <bassmod.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

int main(int argc, char **argv)
{
    struct { uint32_t before; short data[4096]; uint32_t after; } audio = {0xA1B2C3D4,{0},0xE5F60718};
    DWORD received, total = 0, position;
    int device, result = 1;
    FILE *file = NULL;
    printf("ABI %u %u %u %u\n", (unsigned)sizeof(DWORD), (unsigned)sizeof(BOOL),
           (unsigned)sizeof(HSYNC), (unsigned)sizeof(void *));
    printf("VERSION %u\n", (unsigned)BASSMOD_GetVersion());
    if(argc == 1) return 0;
    device = strcmp(argv[1], "decode") == 0 ? -3 : -1;
    if(!BASSMOD_Init(device, 44100, 0)) {
        printf("INIT_ERROR=%u\n", (unsigned)BASSMOD_ErrorGetCode());
        return 2;
    }
    if(argc < 3 || !BASSMOD_MusicLoad(FALSE, argv[2], 0, 0, BASS_MUSIC_LOOP)) goto cleanup;
    if(device == -3) {
        if(argc != 4 || !(file = fopen(argv[3], "wb"))) goto cleanup;
        while(total < 1024 * 1024) {
            received = BASSMOD_MusicDecode(audio.data, sizeof(audio.data));
            if(received == (DWORD)-1 || received == 0 || received > sizeof(audio.data)) goto cleanup;
            if(audio.before != 0xA1B2C3D4 || audio.after != 0xE5F60718) goto cleanup;
            if(fwrite(audio.data, 1, received, file) != received) goto cleanup;
            total += received;
        }
        printf("BASSMOD_C_DECODE=PASS bytes=%u\n", (unsigned)total);
    }
    else {
        if(!BASSMOD_MusicPlay()) goto cleanup;
#ifdef _WIN32
        Sleep(1200);
#else
        { struct timespec delay = {1,200000000}; nanosleep(&delay,NULL); }
#endif
        position = BASSMOD_MusicGetPosition();
        if(BASSMOD_MusicIsActive() != BASS_ACTIVE_PLAYING || position == 0 || position == (DWORD)-1) goto cleanup;
        if(!BASSMOD_MusicStop()) goto cleanup;
        printf("BASSMOD_C_OUTPUT=PASS position=%u\n", (unsigned)position);
    }
    result = 0;
cleanup:
    if(result != 0) printf("RUNTIME_ERROR=%u\n", (unsigned)BASSMOD_ErrorGetCode());
    if(file && fclose(file) != 0) result = 1;
    BASSMOD_MusicFree();
    BASSMOD_Free();
    return result;
}
/* end of bassmod-native.c */
