/*
    FreeBASIC native BASS qualification
    File: bass-abi.c
    Purpose: Establish storage and field offsets from the vendor C header.
    Responsibilities: Native handle widths, public record layouts and SDK version.
    This file contains no playback, recorder or replacement BASS implementation.
*/
#include <bass.h>
#include <stddef.h>
#include <stdio.h>
int main(void)
{
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(DWORD), sizeof(QWORD), sizeof(BOOL), sizeof(HSTREAM),
           sizeof(BASS_DEVICEINFO), offsetof(BASS_DEVICEINFO, flags),
           /* BASS 2.4.18 reserves the old DirectSound capability slots.
              The binding's legacy eax field occupies the last such slot. */
           sizeof(BASS_INFO), offsetof(BASS_INFO, reserved) + 6 * sizeof(DWORD), offsetof(BASS_INFO, freq),
           sizeof(BASS_RECORDINFO), offsetof(BASS_RECORDINFO, freq),
           sizeof(BASS_SAMPLE), offsetof(BASS_SAMPLE, mindist), offsetof(BASS_SAMPLE, outvol),
           sizeof(BASS_CHANNELINFO), offsetof(BASS_CHANNELINFO, filename),
           sizeof(BASS_PLUGINFORM), offsetof(BASS_PLUGINFORM, name),
           sizeof(BASS_PLUGININFO), offsetof(BASS_PLUGININFO, formats),
           sizeof(BASS_3DVECTOR), sizeof(BASS_FILEPROCS),
           sizeof(BASS_DX8_CHORUS), sizeof(BASS_DX8_COMPRESSOR),
           sizeof(BASS_DX8_I3DL2REVERB), sizeof(BASS_DX8_PARAMEQ), sizeof(BASS_DX8_REVERB));
    printf("CONSTANTS %u %u %u %u %u\n", BASSVERSION, BASS_STREAM_DECODE,
           BASS_SAMPLE_FLOAT, BASS_POS_BYTE, BASS_ERROR_HANDLE);
    printf("VERSION %u\n", (unsigned)BASS_GetVersion());
    return 0;
}
/* end of bass-abi.c */
