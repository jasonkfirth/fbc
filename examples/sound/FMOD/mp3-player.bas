'' FreeBASIC FMOD example
'' File: mp3-player.bas
'' Purpose: Play an MP3 and read its optional ID3 metadata.
'' Responsibilities: File errors, bounded tag text and owned stream cleanup.
'' This example contains no MPEG decoder or audio driver implementation.
'' --------

#include once "fmod.bi"
#include once "fmod_tags.bi"

Dim As String sound_file = Command(1)
If sound_file = "" Then
    Print "Usage: mp3-player <audio-file>"
    End 1
End If

If (FSOUND_GetVersion() < FMOD_VERSION) Then
    Print "FMOD version mismatch"
    End 1
End If
If (FSOUND_Init(44100, 4, 0) = 0) Then
    Print "Could not initialize FMOD: "; FSOUND_GetError()
    End 1
End If
If FSOUND_Stream_SetBufferSize(50) = 0 Then
    FSOUND_Close()
    End 1
End If

Dim As FSOUND_STREAM Ptr fmod_stream = FSOUND_Stream_Open(StrPtr(sound_file), FSOUND_MPEGACCURATE, 0, 0)
If (fmod_stream = 0) Then
    Print "FMOD could not load '" & sound_file & "'"
    FSOUND_Close()
    End 1
End If

'' Copy and decode the borrowed metadata while the stream remains open.
'' An absent or invalid text field yields an empty string.
Print "Title:", get_tag(fmod_stream, "TITLE", "TIT2")
Print "Album:", get_tag(fmod_stream, "ALBUM", "TALB")
Print "Artist:", get_tag(fmod_stream, "ARTIST", "TPE1")
''print_all_tags(fmod_stream)

Dim As Long stream_channel = FSOUND_Stream_Play(FSOUND_FREE, fmod_stream)
If stream_channel < 0 Then
    Print "Could not start playback: "; FSOUND_GetError()
    FSOUND_Stream_Close(fmod_stream)
    FSOUND_Close()
    End 1
End If

Dim As Integer playback_error = 0
If Environ("FB_EXAMPLE_AUTOPLAY") = "1" Then
    Sleep 1200, 1
    If FSOUND_Stream_GetPosition(fmod_stream) = 0 Then playback_error = 1
Else
    Print "Playing MP3, press a key to exit..."
    While Inkey() = ""
        If FSOUND_IsPlaying(stream_channel) = 0 Then Exit While
        Sleep 50, 1
    Wend
End If
If FSOUND_IsPlaying(stream_channel) Then
    If FSOUND_Stream_Stop(fmod_stream) = 0 Then playback_error = 1
End If
If FSOUND_Stream_Close(fmod_stream) = 0 Then playback_error = 1
FSOUND_Close()
If playback_error Then End 1
Print "FMOD playback completed."

'' end of mp3-player.bas
