'' FreeBASIC native BASS qualification
'' File: bass-decode.bas
'' Purpose: Decode genuine SDK streams and modules for independent PCM comparison.
'' Responsibilities: Bounded native reads, guard checks and owned resources.
'' This file contains no playback or substitute audio decoder.
#include once "bass.bi"

Const SAMPLE_CAPACITY = 4096
Const BUFFER_BYTES = SAMPLE_CAPACITY * SizeOf(Single)
Type GuardedAudio
    before_value As ULong
    samples(0 To SAMPLE_CAPACITY - 1) As Single
    after_value As ULong
End Type

Dim As GuardedAudio audio
audio.before_value = &hA1B2C3D4
audio.after_value = &hE5F60718
Dim As String input_file = Command(1), output_file = Command(2)
Dim As Integer is_module = (Command(3) = "music")
Dim As DWORD audio_handle, total, received
Dim As Integer output_number, result = 1
'' Match the C probe's bounded partial module reference.
Const LIMIT_BYTES = 4 * 1024 * 1024
If input_file = "" OrElse output_file = "" Then End 1
If BASS_Init(0, 44100, 0, 0, 0) = 0 Then End 2
If is_module Then
    audio_handle = BASS_MusicLoad(0, StrPtr(input_file), 0, 0, _
                            BASS_MUSIC_DECODE Or BASS_SAMPLE_FLOAT Or BASS_MUSIC_PRESCAN, 0)
Else
    audio_handle = BASS_StreamCreateFile(0, StrPtr(input_file), 0, 0, BASS_STREAM_DECODE Or BASS_SAMPLE_FLOAT)
End If
If audio_handle = 0 Then GoTo cleanup
output_number = FreeFile()
If Open(output_file For Binary Access Write As #output_number) <> 0 Then
    output_number = 0
    GoTo cleanup
End If
While total < LIMIT_BYTES
    '' BASS takes a byte count. SIZEOF on this UDT array member gives the
    '' element width, so derive capacity explicitly from its declaration.
    received = BASS_ChannelGetData(audio_handle, @audio.samples(0), BUFFER_BYTES)
    If audio.before_value <> &hA1B2C3D4 OrElse audio.after_value <> &hE5F60718 Then GoTo cleanup
    If received = Cast(DWORD, -1) Then
        If BASS_ErrorGetCode() <> BASS_ERROR_ENDED Then GoTo cleanup
        Exit While
    End If
    If received = 0 OrElse received > BUFFER_BYTES Then GoTo cleanup
    If received Mod SizeOf(Single) <> 0 Then GoTo cleanup
    If Put(#output_number, , audio.samples(0), received \ SizeOf(Single)) <> 0 Then GoTo cleanup
    total += received
Wend
If total = 0 OrElse (is_module = 0 AndAlso total >= LIMIT_BYTES) Then GoTo cleanup
result = 0
cleanup:
If result <> 0 Then
    Print "BASS decode failure: bytes="; total; " last="; received; " module="; is_module; " error="; BASS_ErrorGetCode()
    Print "BASS decode state: handle="; audio_handle; " output="; output_number; " file error="; Err
End If
If output_number <> 0 Then
    If Close(output_number) <> 0 Then result = 1
End If
If audio_handle <> 0 Then
    If is_module Then
        If BASS_MusicFree(audio_handle) = 0 Then result = 1
    Else
        If BASS_StreamFree(audio_handle) = 0 Then result = 1
    End If
End If
If BASS_Free() = 0 Then result = 1
If result = 0 Then
    Print "BASS_FB_DECODE=PASS bytes="; total
Else
    Print "BASS decode cleanup failure: "; BASS_ErrorGetCode()
End If
End result
'' end of bass-decode.bas
