'' FreeBASIC FMOD 3.75 native qualification
'' File: fmod-native.bas
'' Purpose: Compare the vendor ABI and native output with the C probe.
'' Responsibilities: Guarded tag outputs, bounded playback and resource cleanup.
'' This file contains no replacement decoder or audio driver implementation.
'' --------

#lang "fb"
#include once "fmod.bi"

Type TagCounter
    before As ULong
    count As Long
    after As ULong
End Type

Print "ABI "; SizeOf(Any Ptr); " "; SizeOf(FMOD_ERRORS); " "; SizeOf(FSOUND_OUTPUTTYPES); " "; _
    SizeOf(FSOUND_REVERB_PROPERTIES); " "; OffsetOf(FSOUND_REVERB_PROPERTIES, ReflectionsPan(0)); " "; _
    OffsetOf(FSOUND_REVERB_PROPERTIES, Flags); " "; SizeOf(FSOUND_REVERB_CHANNELPROPERTIES); " "; _
    OffsetOf(FSOUND_REVERB_CHANNELPROPERTIES, Flags); " "; SizeOf(FSOUND_TOC_TAG); " "; _
    OffsetOf(FSOUND_TOC_TAG, frame(0)); " "; FSOUND_OUTPUT_ALSA; " "; FSOUND_TAGFIELD_ID3V2
Print "VERSION "; FSOUND_GetVersion()
If Command(1) = "" Then End 0
#ifdef __FB_LINUX__
    '' Use the Linux SDK's default OSS route with a real character device.
    '' Neither no-sound output mode demonstrates native playback support.
    If FSOUND_SetOutput(FSOUND_OUTPUT_OSS) = 0 Then End 2
#endif
If FSOUND_Init(44100, 32, 0) = 0 Then
    Print "INIT_ERROR="; FSOUND_GetError()
    End 2
End If

Dim As String sound_file = Command(2)
Dim As Integer result = 0
If Command(1) = "module" Then
    Dim As FMUSIC_MODULE Ptr song = FMUSIC_LoadSong(StrPtr(sound_file))
    If song = 0 Then
        result = 1
    Else
        If FMUSIC_PlaySong(song) = 0 Then
            result = 1
        Else
            Sleep 1200, 1
            If FMUSIC_IsPlaying(song) = 0 OrElse FMUSIC_GetRow(song) <= 0 Then result = 1
            If FMUSIC_GetTime(song) < 500 OrElse FMUSIC_GetTime(song) > 3000 Then result = 1
            If result = 0 Then Print "FMOD_FB_MODULE=PASS row="; FMUSIC_GetRow(song); " order="; FMUSIC_GetOrder(song); " time="; FMUSIC_GetTime(song)
            If FMUSIC_StopSong(song) = 0 Then result = 1
        End If
        If FMUSIC_FreeSong(song) = 0 Then result = 1
    End If
Else
    Dim As FSOUND_STREAM Ptr fmod_stream = FSOUND_Stream_Open(StrPtr(sound_file), FSOUND_MPEGACCURATE Or FSOUND_LOOP_NORMAL, 0, 0)
    If fmod_stream = 0 Then
        result = 1
    Else
        Dim As TagCounter tags
        tags.before = &hA1B2C3D4
        tags.after = &hE5F60718
        If FSOUND_Stream_GetNumTagFields(fmod_stream, @tags.count) = 0 Then result = 1
        If tags.before <> &hA1B2C3D4 OrElse tags.after <> &hE5F60718 Then result = 1
        If tags.count < 0 OrElse tags.count > 1024 Then result = 1
        Dim As Any Ptr tag_value
        Dim As Long tag_length
        If FSOUND_Stream_FindTagField(fmod_stream, FSOUND_TAGFIELD_ID3V2, "TIT2", @tag_value, @tag_length) Then
            If tag_value = 0 OrElse tag_length <= 0 OrElse tag_length > 4096 Then result = 1
            If result = 0 Then Print "TITLE_BYTES="; tag_length
        End If
        If result = 0 Then
            Dim As Long stream_channel = FSOUND_Stream_Play(FSOUND_FREE, fmod_stream)
            If stream_channel < 0 Then
                result = 1
            Else
                Sleep 1200, 1
                If FSOUND_IsPlaying(stream_channel) = 0 OrElse FSOUND_Stream_GetPosition(fmod_stream) = 0 Then result = 1
                If result = 0 Then Print "FMOD_FB_STREAM=PASS bytes="; FSOUND_Stream_GetPosition(fmod_stream); " tags="; tags.count
                If FSOUND_Stream_Stop(fmod_stream) = 0 Then result = 1
            End If
        End If
        If FSOUND_Stream_Close(fmod_stream) = 0 Then result = 1
    End If
End If
FSOUND_Close()
End result

'' end of fmod-native.bas
