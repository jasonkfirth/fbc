'' FreeBASIC native BASS qualification
'' File: bass-abi.bas
'' Purpose: Compare FreeBASIC storage with the vendor's native C records.
'' Responsibilities: Handle widths, record offsets, flags and linked SDK version.
'' This file contains no playback or substitute BASS implementation.
#include once "bass.bi"

Print "ABI "; SizeOf(DWORD); " "; SizeOf(QWORD); " "; SizeOf(BOOL); " "; SizeOf(HSTREAM); " "; _
    SizeOf(BASS_DEVICEINFO); " "; OffsetOf(BASS_DEVICEINFO, flags); " "; _
    SizeOf(BASS_INFO); " "; OffsetOf(BASS_INFO, eax); " "; OffsetOf(BASS_INFO, freq); " "; _
    SizeOf(BASS_RECORDINFO); " "; OffsetOf(BASS_RECORDINFO, freq); " "; _
    SizeOf(BASS_SAMPLE); " "; OffsetOf(BASS_SAMPLE, mindist); " "; OffsetOf(BASS_SAMPLE, outvol); " "; _
    SizeOf(BASS_CHANNELINFO); " "; OffsetOf(BASS_CHANNELINFO, filename); " "; _
    SizeOf(BASS_PLUGINFORM); " "; OffsetOf(BASS_PLUGINFORM, name); " "; _
    SizeOf(BASS_PLUGININFO); " "; OffsetOf(BASS_PLUGININFO, formats); " "; _
    SizeOf(BASS_3DVECTOR); " "; SizeOf(BASS_FILEPROCS); " "; _
    SizeOf(BASS_DX8_CHORUS); " "; SizeOf(BASS_DX8_COMPRESSOR); " "; _
    SizeOf(BASS_DX8_I3DL2REVERB); " "; SizeOf(BASS_DX8_PARAMEQ); " "; SizeOf(BASS_DX8_REVERB)
Print "CONSTANTS "; BASSVERSION; " "; BASS_STREAM_DECODE; " "; _
    BASS_SAMPLE_FLOAT; " "; BASS_POS_BYTE; " "; BASS_ERROR_HANDLE
Print "VERSION "; BASS_GetVersion()
'' end of bass-abi.bas
