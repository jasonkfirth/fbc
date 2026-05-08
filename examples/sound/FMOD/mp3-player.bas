'' mp3 player based on FMOD

#include once "fmod.bi"

Const SOUND_FILE = "test.mp3"

Sub print_all_tags(ByVal fmod_stream As FSOUND_STREAM Ptr)
    Dim As Integer count = 0
    FSOUND_Stream_GetNumTagFields(fmod_stream, @count)

    For i As Integer = 0 To (count - 1)
        Dim As Integer tagtype, taglen
        Dim As ZString Ptr tagname, tagvalue
        FSOUND_Stream_GetTagField(fmod_stream, i, @tagtype, @tagname, @tagvalue, @taglen)
        Print Left(*tagname, taglen)
    Next
End Sub

Function get_tag _
    ( _
        ByVal fmod_stream As FSOUND_STREAM Ptr, _
        ByVal tagv1 As ZString Ptr, _
        ByVal tagv2 As ZString Ptr _
    ) As String

    Dim tagname As ZString Ptr, taglen As Integer

    FSOUND_Stream_FindTagField(fmod_stream, FSOUND_TAGFIELD_ID3V1, tagv1, @tagname, @taglen)
    If (taglen = 0) Then
        FSOUND_Stream_FindTagField(fmod_stream, FSOUND_TAGFIELD_ID3V2, tagv2, @tagname, @taglen)
    End If

    Return Left(*tagname, taglen)
End Function

    If (FSOUND_GetVersion < FMOD_VERSION) Then
        Print "FMOD version " + Str(FMOD_VERSION) + " or greater required!"
        End 1
    End If

    If (FSOUND_Init(44100, 4, 0) = 0) Then
        Print "Could not initialize FMOD"
        End 1
    End If

    FSOUND_Stream_SetBufferSize(50)

    Dim As FSOUND_STREAM Ptr fmod_stream = FSOUND_Stream_Open(SOUND_FILE, FSOUND_MPEGACCURATE, 0, 0)
    If (fmod_stream = 0) Then
        Print "FMOD could not load '" & SOUND_FILE & "'"
        FSOUND_Close()
        End 1
    End If

    '' Read out mp3 tags to show some meta information
    Print "Title:", get_tag(fmod_stream, "TITLE", "TIT2")
    Print "Album:", get_tag(fmod_stream, "ALBUM", "TALB")
    Print "Artist:", get_tag(fmod_stream, "ARTIST", "TPE1")
    ''print_all_tags(fmod_stream)

    Print "Playing mp3, press a key to exit..."
    FSOUND_Stream_Play(FSOUND_FREE, fmod_stream)

    While (Inkey() = "")
        If (FSOUND_Stream_GetPosition(fmod_stream) >= FSOUND_Stream_GetLength(fmod_stream)) Then
            Exit While
        End If
        Sleep 50, 1
    Wend
   
    FSOUND_Stream_Stop(fmod_stream)
    FSOUND_Stream_Close(fmod_stream)
    FSOUND_Close()
